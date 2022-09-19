local project_title = "3_tutorial_vk_camera"

project (project_title)
  console_app_header(project_title)
  files { "src/**.h", "src/**.c", "../external/rglfw.c", "../external/volk/volk.c" }
  includedirs { "../external/", "../shared/", "$(VULKAN_SDK)/Include" } -- "../external/", "../shared/",
