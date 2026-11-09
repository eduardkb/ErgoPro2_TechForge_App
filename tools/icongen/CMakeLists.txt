# Developer tool: renders resources/icons/app/app.svg into the platform icon
# files (.icns via iconutil, .ico, Linux PNG). Built only with
# -DERGOPRO2_BUILD_TOOLS=ON; used by scripts/generate-icons.sh.
qt_add_executable(ergopro2_icongen main.cpp)
target_link_libraries(ergopro2_icongen PRIVATE Qt6::Gui Qt6::Svg ergopro2_warnings)
