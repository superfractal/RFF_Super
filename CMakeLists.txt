# Modified by AI; earlier exact modification date unavailable.
# Modified by GPT-5 on 2026-08-18, 2026-08-21, 2026-08-26, 2026-09-01.
# Modified by Opus 5 on 2026-08-13, 2026-08-14, 2026-08-15, 2026-08-18, 2026-08-20, 2026-08-21, 2026-08-26, 2026-08-31, 2026-09-01, 2026-09-03

cmake_minimum_required(VERSION 4.1.2)
project(RFF_Super)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(
        RFF_Super
        res/resources.rc
        src/rff2/ui/Main.cpp
        src/rff2/calc/fp_complex_calculator.cpp
        src/rff2/calc/fp_complex_calculator.h
        src/rff2/constants/Constants.hpp
        src/rff2/attr/Attribute.h
        src/rff2/attr/FractalAttribute.h
        src/rff2/attr/RenderAttribute.h
        src/rff2/attr/ShaderAttribute.h
        src/rff2/attr/ShdSlopeAttribute.h
        src/rff2/attr/ShdStripeAttribute.h
        src/rff2/attr/ShdColorAttribute.h
        src/rff2/attr/ShdPaletteAttribute.h
        src/rff2/attr/ShdFogAttribute.h
        src/rff2/attr/VideoAttribute.h
        src/rff2/attr/ShdBloomAttribute.h
        src/rff2/attr/VidDataAttribute.h
        src/rff2/attr/VidExportAttribute.h
        src/rff2/attr/FrtMPAAttribute.h
        src/rff2/calc/fp_complex.cpp
        src/rff2/calc/fp_complex.h
        src/rff2/attr/FrtDecimalizeIterationMethod.h
        src/rff2/attr/FrtReferenceCompAttribute.h
        src/rff2/attr/FrtReuseReferenceMethod.h
        src/rff2/attr/Selectable.h
        src/rff2/calc/fp_decimal_calculator.cpp
        src/rff2/calc/fp_decimal_calculator.h
        src/rff2/calc/fp_decimal.cpp
        src/rff2/calc/fp_decimal.h
        src/rff2/formula/Perturbator.h
        src/rff2/io/RFFDynamicMapBinary.cpp
        src/rff2/io/RFFDynamicMapBinary.h
        src/rff2/io/RFFMapCompression.cpp
        src/rff2/io/RFFMapCompression.h
        src/rff2/data/Matrix.h
        src/rff2/attr/ShdPalColorSmoothingMethod.h
        src/rff2/attr/ShdPalColorInterpolationMethod.h
        src/rff2/attr/ShdSlopeShadingBlend.h
        src/rff2/attr/ShdWarpSource.h
        src/rff2/attr/ShdWarpAttribute.h
        src/rff2/data/ColorUtils.h
        src/rff2/attr/ShdStripeType.h
        src/rff2/attr/VidAnimationAttribute.h
        src/rff2/attr/VidKeyInterpolation.h
        src/rff2/attr/VidTimelineAttribute.h
        src/rff2/attr/VidTimelineSlotSelection.h
        src/rff2/attr/VidTimelineTarget.h
        src/rff2/io/TimelineIO.cpp
        src/rff2/io/TimelineIO.h
        src/rff2/video/TimelineSchedule.cpp
        src/rff2/video/TimelineSchedule.hpp
        src/rff2/video/VideoFrameSource.cpp
        src/rff2/video/VideoFrameSource.hpp
        src/rff2/video/TimelineParams.cpp
        src/rff2/video/TimelineParams.hpp
        src/rff2/video/TimelineEvaluator.cpp
        src/rff2/video/TimelineEvaluator.hpp
        src/rff2/formula/LightMandelbrotReference.cpp
        src/rff2/formula/LightMandelbrotReference.h
        src/rff2/parallel/ParallelRenderState.cpp
        src/rff2/parallel/ParallelRenderState.h
        src/rff2/calc/rff_math.h
        src/rff2/mrthy/ArrayCompressionTool.h
        src/rff2/mrthy/ArrayCompressor.h
        src/rff2/parallel/ParallelArrayDispatcher.h
        src/rff2/ui/Utilities.h
        src/rff2/formula/LightMandelbrotPerturbator.cpp
        src/rff2/formula/CustomFormulaPerturbator.cpp
        src/rff2/formula/ExpressionParser.cpp
        src/rff2/formula/CustomFormulaPerturbator.h
        src/rff2/formula/ExpressionParser.h
        src/rff2/formula/LightMandelbrotPerturbator.h
        src/rff2/mrthy/LightPA.h
        src/rff2/mrthy/LightMPATable.h
        src/rff2/mrthy/MPAPeriod.cpp
        src/rff2/mrthy/MPAPeriod.h
        src/rff2/ui/SettingsWindow.cpp
        src/rff2/ui/SettingsWindow.hpp
        src/rff2/ui/SettingsTheme.hpp
        src/rff2/ui/SettingsMenu.cpp
        src/rff2/ui/SettingsMenu.hpp
        src/rff2/ui/CallbackFile.cpp
        src/rff2/ui/CallbackFile.hpp
        src/rff2/ui/CallbackFractal.cpp
        src/rff2/ui/CallbackFractal.hpp
        src/rff2/ui/CallbackRender.cpp
        src/rff2/ui/CallbackRender.hpp
        src/rff2/ui/CallbackShader.cpp
        src/rff2/ui/CallbackShader.hpp
        src/rff2/locator/MandelbrotLocator.cpp
        src/rff2/locator/MandelbrotLocator.h
        src/rff2/ui/CallbackExplore.cpp
        src/rff2/ui/CallbackExplore.hpp
        src/rff2/ui/CallbackDebug.cpp
        src/rff2/ui/CallbackDebug.hpp
        src/rff2/calc/dex.h
        src/rff2/formula/DeepMandelbrotReference.cpp
        src/rff2/formula/DeepMandelbrotReference.h
        src/rff2/formula/MandelbrotReference.h
        src/rff2/calc/dex_std.h
        src/rff2/calc/double_exp_math.h
        src/rff2/formula/DeepMandelbrotPerturbator.cpp
        src/rff2/formula/DeepMandelbrotPerturbator.h
        src/rff2/formula/MandelbrotPerturbator.h
        src/rff2/mrthy/DeepPA.h
        src/rff2/mrthy/DeepMPATable.h
        src/rff2/mrthy/MPATable.h
        src/rff2/mrthy/PA.h
        src/rff2/ui/Callback.hpp
        src/rff2/mrthy/PAGenerator.h
        src/rff2/mrthy/LightPAGenerator.cpp
        src/rff2/mrthy/LightPAGenerator.h
        src/rff2/mrthy/DeepPAGenerator.cpp
        src/rff2/mrthy/DeepPAGenerator.h
        src/rff2/data/ApproxTableCache.h
        src/rff2/preset/shader/palette/ShdPalettePresets.h
        src/rff2/preset/shader/color/ShdColorPresets.h
        src/rff2/preset/shader/bloom/ShdBloomPresets.h
        src/rff2/preset/shader/fog/ShdFogPresets.h
        src/rff2/preset/shader/slope/ShdSlopePresets.h
        src/rff2/preset/shader/stripe/ShdStripePresets.h
        src/rff2/preset/shader/example/ShdExamplePresets.h
        src/rff2/preset/shader/palette/ShdPalettePresets.cpp
        src/rff2/preset/shader/fog/ShdFogPresets.cpp
        src/rff2/preset/shader/color/ShdColorPresets.cpp
        src/rff2/preset/shader/slope/ShdSlopePresets.cpp
        src/rff2/preset/shader/stripe/ShdStripePresets.cpp
        src/rff2/preset/shader/bloom/ShdBloomPresets.cpp
        src/rff2/preset/shader/example/ShdExamplePresets.cpp
        src/rff2/preset/Presets.h
        src/rff2/preset/resolution/ResolutionPresets.cpp
        src/rff2/preset/resolution/ResolutionPresets.h
        src/rff2/preset/render/RenderPresets.cpp
        src/rff2/preset/render/RenderPresets.h
        src/rff2/preset/calc/CalculationPresets.cpp
        src/rff2/preset/calc/CalculationPresets.h
        src/rff2/ui/CallbackVideo.cpp
        src/rff2/ui/CallbackVideo.hpp
        src/rff2/ui/IOUtilities.cpp
        src/rff2/ui/IOUtilities.h
        src/rff2/parallel/BackgroundThreads.h
        src/rff2/parallel/BackgroundThread.h
        src/rff2/io/RFFStaticMapBinary.cpp
        src/rff2/io/RFFStaticMapBinary.h
        src/rff2/io/RFFBinary.cpp
        src/rff2/io/RFFBinary.h
        src/rff2/io/RFFLocationBinary.cpp
        src/rff2/io/RFFLocationBinary.h
        src/rff2/io/KFRColorLoader.cpp
        src/rff2/io/KFRColorLoader.hpp
        src/rff2/io/ShaderPresetIO.cpp
        src/rff2/io/ShaderPresetIO.h
        src/rff2/io/ConfigIO.cpp
        src/rff2/io/ConfigIO.h
        src/rff2/io/RecoveryIO.cpp
        src/rff2/io/RecoveryIO.h
        src/rff2/io/PreferencesIO.cpp
        src/rff2/io/PreferencesIO.h
        src/rff2/ui/RecoveryPrompt.cpp
        src/rff2/ui/RecoveryPrompt.hpp
        src/rff2/ui/ImageCanvas.cpp
        src/rff2/ui/ImageCanvas.hpp
        extern/stb_image.c
        extern/stb_image.h
        src/vulkan_helper/handle/Handler.hpp
        src/vulkan_helper/impl/GraphicsContextWindow.cpp
        src/vulkan_helper/impl/GraphicsContextWindow.hpp
        src/vulkan_helper/impl/ValidationLayer.cpp
        src/vulkan_helper/impl/ValidationLayer.hpp
        src/vulkan_helper/core/exception.hpp
        src/vulkan_helper/impl/Instance.cpp
        src/vulkan_helper/impl/Instance.hpp
        src/vulkan_helper/impl/Surface.cpp
        src/vulkan_helper/impl/Surface.hpp
        src/vulkan_helper/impl/PhysicalDeviceLoader.cpp
        src/vulkan_helper/impl/PhysicalDeviceLoader.hpp
        src/vulkan_helper/struct/QueueFamilyIndices.hpp
        src/vulkan_helper/impl/LogicalDevice.cpp
        src/vulkan_helper/impl/LogicalDevice.hpp
        src/vulkan_helper/struct/StringHasher.hpp
        src/vulkan_helper/impl/Swapchain.cpp
        src/vulkan_helper/impl/Swapchain.hpp
        src/vulkan_helper/impl/CommandPool.cpp
        src/vulkan_helper/impl/CommandPool.hpp
        src/vulkan_helper/impl/CommandBuffer.cpp
        src/vulkan_helper/impl/CommandBuffer.hpp
        src/vulkan_helper/impl/Descriptor.cpp
        src/vulkan_helper/impl/Descriptor.hpp
        src/vulkan_helper/manage/HostDataObjectManager.hpp
        src/vulkan_helper/impl/BufferObject.cpp
        src/vulkan_helper/impl/BufferObject.hpp
        src/vulkan_helper/util/BufferImageUtils.hpp
        src/vulkan_helper/manage/DescriptorManager.hpp
        src/vulkan_helper/impl/Pipeline.hpp
        src/vulkan_helper/impl/RenderPass.cpp
        src/vulkan_helper/impl/RenderPass.hpp
        src/vulkan_helper/manage/PipelineManager.hpp
        src/vulkan_helper/impl/DescriptorSetLayout.hpp
        src/vulkan_helper/struct/DescriptorSetLayoutBuildType.hpp
        src/vulkan_helper/impl/PipelineLayout.hpp
        src/vulkan_helper/impl/DescriptorSetLayout.cpp
        src/vulkan_helper/impl/PipelineLayout.cpp
        src/vulkan_helper/impl/PushConstant.hpp
        src/vulkan_helper/impl/ShaderModule.hpp
        src/vulkan_helper/impl/ShaderModule.cpp
        src/vulkan_helper/manage/RenderPassManager.hpp
        src/vulkan_helper/struct/RenderPassAttachmentType.hpp
        src/vulkan_helper/impl/Core.cpp
        src/vulkan_helper/impl/Core.hpp
        src/vulkan_helper/handle/CoreHandler.hpp
        src/vulkan_helper/impl/Framebuffer.cpp
        src/vulkan_helper/impl/Framebuffer.hpp
        src/vulkan_helper/struct/RenderPassAttachment.hpp
        src/vulkan_helper/struct/ImageInitInfo.hpp
        src/vulkan_helper/impl/SyncObject.cpp
        src/vulkan_helper/impl/SyncObject.hpp
        src/vulkan_helper/executor/RenderPassFullscreenRecorder.cpp
        src/vulkan_helper/executor/RenderPassFullscreenRecorder.hpp
        src/vulkan_helper/struct/Vertex.hpp
        src/vulkan_helper/impl/VertexBuffer.cpp
        src/vulkan_helper/impl/VertexBuffer.hpp
        src/vulkan_helper/impl/HostDataObject.hpp
        src/vulkan_helper/impl/Uniform.cpp
        src/vulkan_helper/impl/Uniform.hpp
        src/vulkan_helper/repo/GlobalShaderModuleRepo.hpp
        src/vulkan_helper/context/WindowContext.hpp
        src/vulkan_helper/context/RenderContext.hpp
        src/vulkan_helper/hash/VectorHasher.hpp
        src/vulkan_helper/hash/PointerDerefEquals.hpp
        src/vulkan_helper/repo/Repository.hpp
        src/vulkan_helper/repo/Repositories.hpp
        src/vulkan_helper/repo/RepoAbstract.hpp
        src/vulkan_helper/manage/PipelineLayoutManager.hpp
        src/vulkan_helper/hash/BoostHasher.hpp
        src/vulkan_helper/repo/GlobalPipelineLayoutRepo.hpp
        src/vulkan_helper/impl/IndexBuffer.cpp
        src/vulkan_helper/impl/IndexBuffer.hpp
        src/vulkan_helper/impl/CombinedImageSampler.cpp
        src/vulkan_helper/impl/CombinedImageSampler.hpp
        src/vulkan_helper/configurator/GeneralGraphicsPipelineConfigurator.cpp
        src/vulkan_helper/configurator/GeneralGraphicsPipelineConfigurator.hpp
        src/vulkan_helper/repo/WindowLocalDescriptorRepo.hpp
        src/vulkan_helper/struct/DescriptorTemplateInfo.hpp
        src/rff2/vulkan/SharedDescriptorTemplate.hpp
        src/rff2/vulkan/ShaderAnimationPhases.hpp
        src/vulkan_helper/impl/Engine.cpp
        src/vulkan_helper/impl/Engine.hpp
        src/vulkan_helper/executor/ScopedNewCommandBufferExecutor.cpp
        src/vulkan_helper/executor/ScopedNewCommandBufferExecutor.hpp
        src/vulkan_helper/struct/PushConstantReserve.hpp
        src/vulkan_helper/configurator/RenderContextConfigurator.hpp
        src/rff2/vulkan/RCC1.hpp
        src/vulkan_helper/hash/DescriptorSetLayoutBuildTypeHasher.hpp
        src/vulkan_helper/context/ImageContext.hpp
        src/vulkan_helper/util/SwapchainUtils.hpp
        src/rff2/vulkan/GPCIterationPalette.cpp
        src/rff2/vulkan/GPCIterationPalette.hpp
        src/rff2/vulkan/PaletteBandLine.hpp
        src/rff2/vulkan/TextureDescriptor.cpp
        src/rff2/vulkan/TextureDescriptor.hpp
        src/rff2/attr/ShdTextureAttribute.h
        src/rff2/attr/ShdTextureBlendMode.h
        src/rff2/attr/ShdTextureUVMode.h
        src/vulkan_helper/context/DescriptorUpdateContext.hpp
        src/vulkan_helper/core/safe_array.hpp
        src/vulkan_helper/util/BufferImageContextUtils.hpp
        src/vulkan_helper/configurator/GeneralPostProcessGraphicsPipelineConfigurator.cpp
        src/vulkan_helper/configurator/GeneralPostProcessGraphicsPipelineConfigurator.hpp
        src/rff2/ui/RenderScene.cpp
        src/rff2/ui/RenderScene.hpp
        src/rff2/ui/Application.cpp
        src/rff2/ui/Application.hpp
        src/rff2/constants/Win32Constants.hpp
        src/rff2/constants/FractalConstants.hpp
        src/rff2/constants/NumConstants.hpp
        src/rff2/constants/NullConstants.hpp
        src/rff2/constants/VideoConstants.hpp
        src/rff2/constants/ExtensionConstants.hpp
        src/rff2/constants/StatusConstants.hpp
        src/vulkan_helper/configurator/GraphicsPipelineConfigurator.hpp
        src/rff2/constants/VulkanWindowConstants.hpp
        src/vulkan_helper/struct/DescriptorTemplate.hpp
        src/vulkan_helper/impl/Sampler.cpp
        src/vulkan_helper/impl/Sampler.hpp
        src/vulkan_helper/repo/GlobalSamplerRepo.hpp
        src/vulkan_helper/hash/SamplerCreateInfoHasher.hpp
        src/vulkan_helper/hash/SamplerCreateInfoEquals.hpp
        src/vulkan_helper/impl/ShaderStorage.cpp
        src/vulkan_helper/impl/ShaderStorage.hpp
        src/vulkan_helper/struct/BufferLock.hpp
        src/vulkan_helper/core/logger.hpp
        src/vulkan_helper/util/DescriptorUpdater.hpp
        src/rff2/vulkan/GPCStripe.cpp
        src/rff2/vulkan/GPCStripe.hpp
        src/rff2/vulkan/GPCSlope.cpp
        src/rff2/vulkan/GPCSlope.hpp
        src/rff2/vulkan/GPCColor.cpp
        src/rff2/vulkan/GPCColor.hpp
        src/rff2/vulkan/GPCFog.cpp
        src/rff2/vulkan/GPCFog.hpp
        src/rff2/vulkan/GPCDownsampleForBlur.cpp
        src/rff2/vulkan/GPCDownsampleForBlur.hpp
        src/rff2/vulkan/GPCBloomThreshold.cpp
        src/rff2/vulkan/GPCBloomThreshold.hpp
        src/vulkan_helper/core/factory.hpp
        src/vulkan_helper/util/Debugger.hpp
        src/vulkan_helper/util/PhysicalDeviceUtils.hpp
        src/vulkan_helper/util/GraphicsContextWindowProc.hpp
        src/vulkan_helper/impl/ComputeShaderPipeline.cpp
        src/vulkan_helper/impl/ComputeShaderPipeline.hpp
        src/vulkan_helper/impl/GraphicsPipeline.cpp
        src/vulkan_helper/impl/GraphicsPipeline.hpp
        src/vulkan_helper/configurator/PipelineConfigurator.hpp
        src/vulkan_helper/executor/ScopedCommandBufferExecutor.cpp
        src/vulkan_helper/executor/ScopedCommandBufferExecutor.hpp
        src/vulkan_helper/configurator/ComputePipelineConfigurator.cpp
        src/vulkan_helper/configurator/ComputePipelineConfigurator.hpp
        src/vulkan_helper/struct/InputAttachment.hpp
        src/rff2/vulkan/CPCBoxBlur.cpp
        src/rff2/vulkan/CPCBoxBlur.hpp
        src/rff2/vulkan/RCCPresent.hpp
        src/vulkan_helper/core/config.hpp
        src/vulkan_helper/impl/Fence.cpp
        src/vulkan_helper/impl/Fence.hpp
        src/rff2/vulkan/RCCDownsampleForBlur.hpp
        src/rff2/vulkan/RCC3.hpp
        src/rff2/vulkan/GPCBloom.cpp
        src/rff2/vulkan/GPCBloom.hpp
        src/rff2/vulkan/RCC4.hpp
        src/rff2/vulkan/GPCLinearInterpolation.cpp
        src/rff2/vulkan/GPCLinearInterpolation.hpp
        src/rff2/vulkan/RCC5.hpp
        src/vulkan_helper/core/vkh_base.hpp
        src/vulkan_helper/core/vkh.hpp
        src/vulkan_helper/core/vkh_core.hpp
        src/vulkan_helper/hash/PipelineLayoutManagerEquals.hpp
        src/vulkan_helper/hash/VariantHasher.hpp
        src/vulkan_helper/impl/Semaphore.cpp
        src/vulkan_helper/impl/Semaphore.hpp
        src/vulkan_helper/context/SharedImageContext.hpp
        src/rff2/vulkan/SharedImageContextIndices.hpp
        src/vulkan_helper/util/BarrierUtils.hpp
        src/vulkan_helper/context/BufferContext.hpp
        src/vulkan_helper/struct/BufferInitInfo.hpp
        src/rff2/ui/RenderSceneRequests.hpp
        src/rff2/ui/RenderSceneRenderer.hpp
        src/rff2/vulkan/GPCPresent.cpp
        src/rff2/vulkan/GPCPresent.hpp
        src/rff2/ui/VideoWindow.cpp
        src/rff2/ui/VideoWindow.hpp
        src/rff2/ui/TimelineWindow.cpp
        src/rff2/ui/TimelineWindow.hpp
        src/rff2/ui/NumericExpression.hpp
        src/rff2/ui/VideoRenderScene.cpp
        src/rff2/ui/VideoRenderScene.hpp
        src/rff2/ui/VideoRenderSceneRenderer.hpp
        src/rff2/vulkan/CPC2MapIterationStripe.cpp
        src/rff2/vulkan/CPC2MapIterationStripe.hpp
        src/rff2/vulkan/RCC1Vid.hpp
        src/rff2/vulkan/RCC2Vid.hpp
        src/rff2/vulkan/RCC3Vid.hpp
        src/rff2/vulkan/RCC4Vid.hpp
        src/rff2/vulkan/RCCDownsampleForBlurVid.hpp
        src/vulkan_helper/context/WindowContext.cpp
        src/vulkan_helper/impl/Renderer.hpp
        src/rff2/vulkan/RCC0.hpp
        src/vulkan_helper/core/allocator.hpp
        src/rff2/vulkan/GPCStaticImage2Map.cpp
        src/rff2/vulkan/GPCStaticImage2Map.hpp
        src/rff2/vulkan/CPCImageRGBA2BGR.cpp
        src/rff2/vulkan/CPCImageRGBA2BGR.hpp
        src/rff2/ui/VideoBufferCache.hpp
)

# The Debug binary is a different program - unoptimized, bounds-checked, sanitized where the
# toolchain allows - so it is named apart rather than dropped over the Release one. It still has to
# sit in bin/ itself: the shader loader resolves ../shaders/ from the folder the exe is in.
set_target_properties(RFF_Super PROPERTIES OUTPUT_NAME "RFF_Super" OUTPUT_NAME_DEBUG "RFF_Super_Debug")

find_package(Vulkan REQUIRED)
# Only the three modules the code actually calls; this also pins headers and libraries to one
# OpenCV install instead of letting a hand-placed copy in include/ shadow the linked one.
find_package(OpenCV REQUIRED COMPONENTS core imgcodecs imgproc)

# Debug is a debugging build and nothing else: nothing optimized away, full symbols, and a stack
# that can always be walked back to the frame that did it. AddressSanitizer is asked for here too,
# but probed rather than assumed: the MSYS2 mingw-w64 GCC this is normally built with ships no
# libasan, and hard-coding the flag would turn every Debug build on that toolchain into a link
# error. Configure with the MSYS2 clang64 toolchain to get the sanitizer itself.
option(RFF_ASAN "Use AddressSanitizer in the Debug configuration when the toolchain can link it" ON)

set(RFF_DEBUG_COMPILE_OPTIONS -O0 -g3 -fno-omit-frame-pointer -fno-optimize-sibling-calls)
set(RFF_DEBUG_LINK_OPTIONS "")

if (RFF_ASAN)
    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_FLAGS -fsanitize=address)
    set(CMAKE_REQUIRED_LINK_OPTIONS -fsanitize=address)
    check_cxx_source_compiles("int main() { return 0; }" RFF_ASAN_LINKS)
    unset(CMAKE_REQUIRED_FLAGS)
    unset(CMAKE_REQUIRED_LINK_OPTIONS)
    if (RFF_ASAN_LINKS)
        list(APPEND RFF_DEBUG_COMPILE_OPTIONS -fsanitize=address)
        list(APPEND RFF_DEBUG_LINK_OPTIONS -fsanitize=address)
    else ()
        message(WARNING "RFF_ASAN is on, but this toolchain cannot link -fsanitize=address "
                "(mingw-w64 GCC ships no libasan). Building Debug without it.")
    endif ()
endif ()

target_compile_options(RFF_Super PRIVATE
        #Debug options
        $<$<CONFIG:Debug>:${RFF_DEBUG_COMPILE_OPTIONS}>
        #Release options
        $<$<CONFIG:Release>:-O3 -ffast-math -funroll-loops -march=native>
)

target_link_options(RFF_Super PRIVATE
        $<$<CONFIG:Debug>:${RFF_DEBUG_LINK_OPTIONS}>
)

# Palette generation must stay bit-reproducible so saved recipes ({presetId, seed}) regenerate the
# same colors regardless of build. -Ofast/-march=native enable fast-math and FMA contraction, which
# perturb the float math; disable both for this one TU (appended flags win).
set_source_files_properties(src/rff2/preset/shader/palette/ShdPalettePresets.cpp PROPERTIES
        COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off"
)

target_compile_definitions(RFF_Super
        PRIVATE
        VK_USE_PLATFORM_WIN32_KHR
        # Bounds-checked libstdc++ containers, so an index past the end of a vector stops where it
        # happens instead of reading on. ABI-compatible, unlike _GLIBCXX_DEBUG, so OpenCV and GMP
        # do not have to be rebuilt to match.
        $<$<CONFIG:Debug>:_GLIBCXX_ASSERTIONS>
)

target_include_directories(RFF_Super PRIVATE
        # standard include
        ${CMAKE_SOURCE_DIR}/include
        # extern sources
        ${CMAKE_SOURCE_DIR}/extern
        # vulkan sdk directory
        ${Vulkan_INCLUDE_DIRS}
        # opencv, from the install find_package resolved
        ${OpenCV_INCLUDE_DIRS}
)

target_link_directories(RFF_Super PRIVATE ${CMAKE_SOURCE_DIR}/lib)

target_link_libraries(RFF_Super PRIVATE
        # GMP Linker
        gmp
        # Win32 Linker
        comctl32
        comdlg32
        # Entropy coder of the compressed map format
        zstd
        # OpenCV Linkers
        ${OpenCV_LIBS}
        # Vulkan
        Vulkan::Vulkan
)

add_custom_target(spirv ALL COMMAND powershell -ExecutionPolicy Bypass -File "${CMAKE_CURRENT_SOURCE_DIR}/compile.ps1")
add_dependencies(RFF_Super spirv)
