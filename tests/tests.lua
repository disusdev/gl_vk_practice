local project_title = "tests"

project (project_title)
  console_app_header(project_title)
  files { "src/**.h", "src/**.c" }
  includedirs { "../" } -- "../external/", "../shared/", 