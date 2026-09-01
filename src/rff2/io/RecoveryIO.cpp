//
// Created by Opus 5 on 2026-08-14.
// Modified by Opus 5 on 2026-08-15.
// Modified by GPT-5 on 2026-08-23, 2026-09-01
//

#include "RecoveryIO.h"

#include <format>
#include <fstream>
#include <windows.h>

#include "ConfigIO.h"
#include "../constants/Constants.hpp"
#include "../ui/IOUtilities.h"
#include "../ui/Utilities.h"

namespace merutilm::rff2 {
    namespace {
        // The lock names its process by id and by when that process started. Windows hands an id out
        // again once its holder is gone, so the start time is what tells this session's lock apart
        // from a dead one that happened to run under the same id.
        struct SessionOwner {
            uint32_t pid;
            uint64_t createdAt;
        };

        constexpr auto LOCK_EXTENSION = L".lock";
        constexpr auto TEMP_EXTENSION = L".tmp";
        constexpr auto CRASHED_NAME = L"crashed";
        constexpr auto INTERRUPTED_NAME = L"interrupted";

        // Files of this session, empty until beginSession().
        std::filesystem::path sessionLock = {};
        std::filesystem::path sessionSnapshot = {};

        std::filesystem::path recoveryDir() {
            return Utilities::getDefaultPath() / L"recovery";
        }

        // The two files a session leaves under a name of its own rather than its lock's: what is
        // being offered, and what a run closed partway kept for the next start.
        std::filesystem::path fixedFile(const wchar_t *name) {
            return recoveryDir() / std::format(L"{}.{}", name, Constants::Extension::CONFIG);
        }

        uint64_t processCreationTime(const HANDLE process) {
            FILETIME creation;
            FILETIME exited;
            FILETIME kernel;
            FILETIME user;
            if (!GetProcessTimes(process, &creation, &exited, &kernel, &user)) {
                return 0;
            }
            return static_cast<uint64_t>(creation.dwHighDateTime) << 32 | creation.dwLowDateTime;
        }

        SessionOwner currentOwner() {
            return {static_cast<uint32_t>(GetCurrentProcessId()), processCreationTime(GetCurrentProcess())};
        }

        // True while that process is still running, and is still the one that wrote the lock.
        bool isOwnerAlive(const SessionOwner &owner) {
            const HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, owner.pid);
            if (process == nullptr) {
                return false;
            }
            const bool alive = processCreationTime(process) == owner.createdAt &&
                               WaitForSingleObject(process, 0) == WAIT_TIMEOUT;
            CloseHandle(process);
            return alive;
        }

        bool readLock(const std::filesystem::path &path, SessionOwner *owner) {
            std::ifstream in(path, std::ios::in | std::ios::binary);
            if (!in.is_open()) {
                return false;
            }
            IOUtilities::readAndDecode(in, &owner->pid);
            IOUtilities::readAndDecode(in, &owner->createdAt);
            return !in.fail();
        }

        // One session's files are all named after its lock: the snapshot beside it, and the two
        // half-written forms a crash can leave in place of either.
        std::filesystem::path snapshotOfLock(const std::filesystem::path &lock) {
            return std::filesystem::path(lock).replace_extension(
                std::format(L".{}", Constants::Extension::CONFIG));
        }

        std::filesystem::path tempOfLock(const std::filesystem::path &lock) {
            return std::filesystem::path(lock).replace_extension(TEMP_EXTENSION);
        }

        std::filesystem::path tempOfLockItself(const std::filesystem::path &lock) {
            return std::filesystem::path(lock) += TEMP_EXTENSION;
        }

        void removeQuietly(const std::filesystem::path &path) {
            std::error_code ec;
            std::filesystem::remove(path, ec);
        }

        bool replaceFile(const std::filesystem::path &source, const std::filesystem::path &target) {
            return MoveFileExW(source.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
        }
    }

    std::optional<RecoveredSnapshot> RecoveryIO::takeSnapshot() {
        const std::filesystem::path dir = recoveryDir();
        std::error_code ec;
        if (!std::filesystem::exists(dir, ec)) {
            return std::nullopt;
        }

        // Everything a session that is no longer running left behind. The newest of their snapshots
        // is the one offered; the rest are swept away with the locks, so the folder cannot grow.
        std::vector<std::filesystem::path> deadLocks;
        std::filesystem::path newest;
        std::filesystem::file_time_type newestTime;

        for (std::filesystem::directory_iterator it(dir, ec), end; it != end; it.increment(ec)) {
            if (ec) {
                break;
            }
            const std::filesystem::path &path = it->path();
            if (path.extension() != LOCK_EXTENSION) {
                continue;
            }
            SessionOwner owner = {};
            if (readLock(path, &owner) && isOwnerAlive(owner)) {
                // Another instance is running right now: its snapshot is not a crash.
                continue;
            }
            deadLocks.push_back(path);
            const std::filesystem::path snapshot = snapshotOfLock(path);
            std::error_code timeEc;
            const std::filesystem::file_time_type time = std::filesystem::last_write_time(snapshot, timeEc);
            if (timeEc) {
                // The session ended before it had anything to keep.
                continue;
            }
            if (newest.empty() || time > newestTime) {
                newest = snapshot;
                newestTime = time;
            }
        }

        const std::filesystem::path crashed = fixedFile(CRASHED_NAME);
        const std::filesystem::path interrupted = fixedFile(INTERRUPTED_NAME);
        std::optional<RecoveredSnapshot> result = std::nullopt;
        bool preserveNewest = false;
        if (!newest.empty()) {
            if (replaceFile(newest, crashed)) {
                result = RecoveredSnapshot{crashed, RecoveryReason::CRASHED};
            } else {
                preserveNewest = true;
            }
        } else if (!deadLocks.empty() && std::filesystem::exists(crashed, ec)) {
            // A run that ended before it had computed anything - it was very likely the settings
            // offered to it that ended it, so the same ones are offered again rather than lost.
            result = RecoveredSnapshot{crashed, RecoveryReason::CRASHED};
        }
        if (result.has_value()) {
            // A crash outranks a run closed partway, and what is not offered now is not held back
            // for a later start: by then it is a view from two runs ago, offered out of nowhere.
            removeQuietly(interrupted);
        } else if (std::filesystem::exists(interrupted, ec)) {
            // Closed by hand while it was still computing. Moved onto the offered name like a crash
            // is, so it is offered once, and so a start it ends itself has the same to fall back on.
            if (replaceFile(interrupted, crashed)) {
                result = RecoveredSnapshot{crashed, RecoveryReason::INTERRUPTED};
            }
        }
        for (const auto &lock: deadLocks) {
            if (preserveNewest && snapshotOfLock(lock) == newest) {
                continue;
            }
            removeQuietly(snapshotOfLock(lock));
            removeQuietly(tempOfLock(lock));
            removeQuietly(tempOfLockItself(lock));
            removeQuietly(lock);
        }
        return result;
    }

    void RecoveryIO::beginSession() {
        const std::filesystem::path dir = recoveryDir();
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        if (ec) {
            return;
        }
        const SessionOwner owner = currentOwner();
        const std::filesystem::path lock = dir / std::format(L"session-{}{}", owner.pid, LOCK_EXTENSION);
        // Written aside and moved into place, so a lock that is there at all is a whole one: a start
        // alongside this one reads it to decide whether this session is live, and half of it names
        // no process.
        const std::filesystem::path temp = tempOfLockItself(lock);
        std::ofstream out(temp, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            return;
        }
        IOUtilities::encodeAndWrite(out, owner.pid);
        IOUtilities::encodeAndWrite(out, owner.createdAt);
        out.close();
        std::filesystem::rename(temp, lock, ec);
        if (ec) {
            removeQuietly(temp);
            return;
        }
        sessionLock = lock;
        sessionSnapshot = snapshotOfLock(lock);
    }

    void RecoveryIO::endSession(const bool unfinished) {
        if (sessionLock.empty()) {
            return;
        }
        removeQuietly(tempOfLock(sessionLock));
        std::error_code ec;
        if (unfinished && std::filesystem::exists(sessionSnapshot, ec)) {
            // The window answered and the shutdown was orderly, but the view it was asked for never
            // arrived: the settings are kept under the one name a start looks for them by, so what
            // was too heavy to wait for can be lowered rather than found again by hand.
            const std::filesystem::path interrupted = fixedFile(INTERRUPTED_NAME);
            if (!replaceFile(sessionSnapshot, interrupted)) {
                // Keep both the snapshot and its dead-session lock so the next start can try again.
                return;
            }
        } else {
            removeQuietly(sessionSnapshot);
        }
        removeQuietly(sessionLock);
        sessionLock.clear();
        sessionSnapshot.clear();
    }

    void RecoveryIO::writeSnapshot(const Attribute &attr, const uint16_t width, const uint16_t height) {
        if (sessionSnapshot.empty()) {
            return;
        }
        const std::filesystem::path temp = tempOfLock(sessionLock);
        if (!ConfigIO::save(temp, attr, width, height)) {
            removeQuietly(temp);
            return;
        }
        if (!replaceFile(temp, sessionSnapshot)) {
            removeQuietly(temp);
        }
    }
}
