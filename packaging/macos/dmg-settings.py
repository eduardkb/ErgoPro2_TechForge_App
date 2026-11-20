# dmgbuild settings for the ErgoPro2 disk image (2.0.0), used by
# scripts/package-macos.sh. The window that opens with the .dmg shows
# ErgoPro2.app on the left, a link to /Applications on the right and the
# background with the arrow (dmg-background.svg): the usual Mac drag-and-drop
# install. dmgbuild writes the Finder layout (.DS_Store) itself, without
# scripting the Finder, so it also works on the CI machines.
#
# Values passed with -D by the script: app, background, volume_icon.

app = defines["app"]  # noqa: F821 (provided by dmgbuild)
background = defines["background"]  # noqa: F821 multi-resolution TIFF (1x + 2x)
icon = defines["volume_icon"]  # noqa: F821 icon of the mounted volume

format = "UDZO"
filesystem = "HFS+"
files = [app]
symlinks = {"Applications": "/Applications"}

# Positions are the icon centres in the 660x400 background.
icon_locations = {"ErgoPro2.app": (165, 200), "Applications": (495, 200)}
icon_size = 128
text_size = 13
default_view = "icon-view"
show_status_bar = False
show_tab_view = False
show_toolbar = False
show_pathbar = False
show_sidebar = False
show_icon_preview = False
include_icon_view_settings = True
arrange_by = None
window_rect = ((200, 160), (660, 400))
