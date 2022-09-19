local project_title = "2_tutorial_gl_camera"

project (project_title)
  console_app_header(project_title)
  files { "src/**.h", "src/**.c", "../external/rglfw.c" }
  includedirs { "../external/", "../shared/" } -- "../external/", "../shared/",
