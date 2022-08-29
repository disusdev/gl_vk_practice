local project_title = "0_tutorial_cubemap"

project (project_title)
  console_app_header(project_title)
  files { "src/**.h", "src/**.c" }
  includedirs { "../external/", "../shared/" } -- "../external/", "../shared/", 