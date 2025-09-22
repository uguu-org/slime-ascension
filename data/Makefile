# Build images and sounds.  Takes ~2 minutes to build and costs ~6M of
# disk space.
#
# Requires gcc, perl, ruby, netpbm, ImageMagick, Inkscape, and ffmpeg.
# gcc also needs libpng, and perl needs libxml.
#
# See svg_to_png.sh for more details on Inkscape.
#
# The reason why we have this separate "data" directory is due to all the
# extra software needed to build it.  The usual workflow is to refresh data
# only once in a while and keep the built output in the "source" directory.
# This means code changes only need commonly available compiler tools.


BUILD_DIR = build
TARGETS = \
	$(BUILD_DIR)/body-table-64-64.png \
	$(BUILD_DIR)/eyes-table-12-12.png \
	$(BUILD_DIR)/meteor-table-64-64.png \
	$(BUILD_DIR)/platform-table-192-240.png \
	$(BUILD_DIR)/spring-table-32-32.png \
	$(BUILD_DIR)/title.png \
	$(BUILD_DIR)/card.png \
	$(BUILD_DIR)/icon.png \
	$(BUILD_DIR)/itch_cover.png \
	$(BUILD_DIR)/in_the_hall_of_the_mountain_king.mp3

# Compiler settings.
CC = gcc
CXX = g++
CFLAGS = -march=native -O2 -Wall -Wextra -pedantic

# FFmpeg options.
FFMPEG = ffmpeg -loglevel fatal

# ......................................................................
# {{{ Primary build artifacts.

all: $(TARGETS)

$(BUILD_DIR)/body-table-64-64.png: $(BUILD_DIR)/t_body.png optimize_png.pl
	perl optimize_png.pl $< > $@

$(BUILD_DIR)/eyes-table-12-12.png: $(BUILD_DIR)/t_eyes.png optimize_png.pl
	perl optimize_png.pl $< > $@

$(BUILD_DIR)/meteor-table-64-64.png: $(BUILD_DIR)/t_meteor.png optimize_png.pl
	perl optimize_png.pl $< > $@

$(BUILD_DIR)/platform-table-192-240.png: $(BUILD_DIR)/t_platform.png optimize_png.pl
	perl optimize_png.pl $< > $@

$(BUILD_DIR)/spring-table-32-32.png: $(BUILD_DIR)/t_spring.png optimize_png.pl
	perl optimize_png.pl $< > $@

$(BUILD_DIR)/title.png: $(BUILD_DIR)/t_title.png optimize_png.pl
	perl optimize_png.pl $< > $@

$(BUILD_DIR)/card.png: $(BUILD_DIR)/t_card.png optimize_png.pl
	perl optimize_png.pl $< > $@

$(BUILD_DIR)/icon.png: $(BUILD_DIR)/t_icon.png optimize_png.pl
	perl optimize_png.pl $< > $@

$(BUILD_DIR)/itch_cover.png: $(BUILD_DIR)/t_itch_cover.png optimize_png.pl
	perl optimize_png.pl $< > $@

$(BUILD_DIR)/in_the_hall_of_the_mountain_king.mp3: sounds/in_the_hall_of_the_mountain_king.flac
	$(FFMPEG) -i $< -ar 22050 -codec:a libmp3lame -map_metadata -1 -y $@

# }}}

# ......................................................................
# {{{ Intermediate files.

# Base image.  All the drawing is done in the same SVG, and we cut various
# parts of it to make various sprite tables.  This minimizes the number of
# times we need to invoke Inkscape.
$(BUILD_DIR)/t_world.svg: world.svg select_layers.pl remove_unused_defs.pl | make_build_dir
	perl select_layers.pl '^world.*' t_world.png $< | perl remove_unused_defs.pl > $@

$(BUILD_DIR)/t_world.png: $(BUILD_DIR)/t_world.svg svg_to_png.sh | make_build_dir
	./svg_to_png.sh $< $@

# Slime images.
$(BUILD_DIR)/t_body.png: $(BUILD_DIR)/t_world.png $(BUILD_DIR)/dither.exe
	convert $< -depth 8 -colorspace Gray +repage -crop 64x512+0+0 png:- | $(BUILD_DIR)/dither.exe - $@

$(BUILD_DIR)/t_eyes.png: $(BUILD_DIR)/t_world.png $(BUILD_DIR)/dither.exe
	convert $< -depth 8 -colorspace Gray +repage -crop 12x444+64+0 png:- | $(BUILD_DIR)/dither.exe - $@

# Falling meteor.
$(BUILD_DIR)/t_meteor.png: $(BUILD_DIR)/t_world.png $(BUILD_DIR)/fs_dither.exe
	convert $< -depth 8 -colorspace Gray +repage -crop 64x1152+128+0 png:- | $(BUILD_DIR)/fs_dither.exe - $@

# Platform images.
$(BUILD_DIR)/t_platform.png: $(BUILD_DIR)/t_platform_gray.png $(BUILD_DIR)/fs_dither.exe
	$(BUILD_DIR)/fs_dither.exe $< $@

$(BUILD_DIR)/t_platform_gray.png: $(BUILD_DIR)/t_platform0.png $(BUILD_DIR)/t_platform1.png $(BUILD_DIR)/t_platform2.png $(BUILD_DIR)/t_platform3.png $(BUILD_DIR)/t_platform4.png $(BUILD_DIR)/t_platform5.png
	convert -size 1152x960 xc:"rgba(0,0,0,0)" -depth 8 -colorspace Gray \
	'(' $(BUILD_DIR)/t_platform0.png +repage -geometry +0+0 ')' -composite \
	'(' $(BUILD_DIR)/t_platform1.png +repage -geometry +192+0 ')' -composite \
	'(' $(BUILD_DIR)/t_platform2.png +repage -geometry +384+0 ')' -composite \
	'(' $(BUILD_DIR)/t_platform3.png +repage -geometry +576+0 ')' -composite \
	'(' $(BUILD_DIR)/t_platform4.png +repage -geometry +768+0 ')' -composite \
	'(' $(BUILD_DIR)/t_platform5.png +repage -geometry +960+0 ')' -composite \
	$@

$(BUILD_DIR)/t_platform0.png: $(BUILD_DIR)/t_world.png
	convert $< -depth 8 -colorspace Gray +repage -crop 192x960+288+0 $@

$(BUILD_DIR)/t_platform1.png: $(BUILD_DIR)/t_world.png
	convert $< -depth 8 -colorspace Gray +repage -crop 192x960+512+0 $@

$(BUILD_DIR)/t_platform2.png: $(BUILD_DIR)/t_world.png
	convert $< -depth 8 -colorspace Gray +repage -crop 160x960+736+0 $@

$(BUILD_DIR)/t_platform3.png: $(BUILD_DIR)/t_world.png
	convert $< -depth 8 -colorspace Gray +repage -crop 160x960+928+0 $@

$(BUILD_DIR)/t_platform4.png: $(BUILD_DIR)/t_world.png
	convert $< -depth 8 -colorspace Gray +repage -crop 128x960+1120+0 $@

$(BUILD_DIR)/t_platform5.png: $(BUILD_DIR)/t_world.png
	convert $< -depth 8 -colorspace Gray +repage -crop 128x960+1280+0 $@

# Spring.
$(BUILD_DIR)/t_spring.png: $(BUILD_DIR)/t_world.png $(BUILD_DIR)/dither.exe
	convert $< -depth 8 -colorspace Gray +repage -crop 32x96+64+512 png:- | $(BUILD_DIR)/dither.exe - $@

# Title logo.
$(BUILD_DIR)/t_title.png: $(BUILD_DIR)/t_title_gray.png $(BUILD_DIR)/dither.exe
	$(BUILD_DIR)/dither.exe $< $@

$(BUILD_DIR)/t_title_gray.png: $(BUILD_DIR)/t_world.png
	convert $< -depth 8 -colorspace Gray +repage -crop 336x48+288+976 $@

# Launcher card.
$(BUILD_DIR)/t_card.png: $(BUILD_DIR)/t_card_bg.png $(BUILD_DIR)/t_card_fg.png
	convert $(BUILD_DIR)/t_card_bg.png '(' $(BUILD_DIR)/t_card_fg.png +repage -geometry +0+0 ')' -composite $@

$(BUILD_DIR)/t_card_fg.png: $(BUILD_DIR)/t_world.png $(BUILD_DIR)/fs_dither.exe
	convert $< -depth 8 -colorspace Gray +repage -crop 350x155+256+1072 png:- | $(BUILD_DIR)/fs_dither.exe - $@

$(BUILD_DIR)/t_card_bg.png: $(BUILD_DIR)/t_world.png $(BUILD_DIR)/dither.exe
	convert $< -depth 8 -colorspace Gray +repage -crop 350x155+256+1584 png:- | $(BUILD_DIR)/dither.exe - $@

# Launcher icon.
$(BUILD_DIR)/t_icon.png: $(BUILD_DIR)/t_world.png $(BUILD_DIR)/dither.exe
	convert $< -depth 8 -colorspace Gray +repage -crop 32x32+0+544 png:- | $(BUILD_DIR)/dither.exe - $@

# itch.io cover.
$(BUILD_DIR)/t_itch_cover.png: $(BUILD_DIR)/t_world.svg $(BUILD_DIR)/t_aa_svg_to_png.sh
	bash $(BUILD_DIR)/t_aa_svg_to_png.sh $< $@ 768 1072 1398 1572

# }}}

# ......................................................................
# {{{ Tools.

# There are various tools here that we don't use.  Basically we just
# copy whole directory whenever we need to create a new project.

# Convert grayscale PNG to black and white.
$(BUILD_DIR)/dither.exe: dither.c | make_build_dir
	$(CC) $(CFLAGS) $< -lpng -o $@

$(BUILD_DIR)/fs_dither.exe: fs_dither.c | make_build_dir
	$(CC) $(CFLAGS) $< -lpng -o $@

$(BUILD_DIR)/random_dither.exe: random_dither.c | make_build_dir
	$(CC) $(CFLAGS) $< -lpng -o $@

# Reduce the sizes of image tables.
$(BUILD_DIR)/crop_table.exe: crop_table.c | make_build_dir
	$(CC) $(CFLAGS) $< -lpng -o $@

$(BUILD_DIR)/shrink_tiles.exe: shrink_tiles.c | make_build_dir
	$(CC) $(CFLAGS) $< -lpng -o $@

# Composite a series of black and white PNGs together.
$(BUILD_DIR)/stack_bw.exe: stack_bw.c | make_build_dir
	$(CC) $(CFLAGS) $< -lpng -o $@

# Find unique tiles to reconstruct the input bitmap.
$(BUILD_DIR)/generate_unique_tiles.exe: generate_unique_tiles.cc | make_build_dir
	$(CXX) $(CFLAGS) $< -lpng -o $@

# SVG rasterizer script with anti-aliasing enabled (i.e. without the flag
# to disable anti-aliasing).
$(BUILD_DIR)/t_aa_svg_to_png.sh: svg_to_png.sh | make_build_dir
	grep -vF "export-png-antialias=0" $< > $@

# }}}

# ......................................................................
# {{{ Tests and maintenance commands.

test: \
	$(BUILD_DIR)/test_passed.brighten \
	$(BUILD_DIR)/test_passed.cleanup_styles \
	$(BUILD_DIR)/test_passed.crop_table \
	$(BUILD_DIR)/test_passed.dither \
	$(BUILD_DIR)/test_passed.element_count \
	$(BUILD_DIR)/test_passed.select_layers \
	$(BUILD_DIR)/test_passed.no_text \
	$(BUILD_DIR)/test_passed.shrink_tiles \
	$(BUILD_DIR)/test_passed.stack_bw \
	$(BUILD_DIR)/test_passed.generate_unique_tiles

$(BUILD_DIR)/test_passed.remove_unused_defs: remove_unused_defs.pl test_remove_unused_defs.sh | make_build_dir
	./test_remove_unused_defs.sh $< && touch $@

$(BUILD_DIR)/test_passed.select_layers: select_layers.pl test_select_layers.sh | make_build_dir
	./test_select_layers.sh $< && touch $@

$(BUILD_DIR)/test_passed.element_count: element_count.pl test_element_count.sh | make_build_dir
	./test_element_count.sh $< && touch $@

$(BUILD_DIR)/test_passed.cleanup_styles: cleanup_styles.pl test_cleanup_styles.sh | make_build_dir
	./test_cleanup_styles.sh $< && touch $@

$(BUILD_DIR)/test_passed.brighten: brighten.pl test_brighten.sh | make_build_dir
	./test_brighten.sh $< && touch $@

$(BUILD_DIR)/test_passed.no_text: $(BUILD_DIR)/t_world.svg element_count.pl
	! ( perl element_count.pl $< | grep '^text' ) && touch $@

$(BUILD_DIR)/test_passed.dither: $(BUILD_DIR)/dither.exe test_dither.sh
	./test_dither.sh $< && touch $@

$(BUILD_DIR)/test_passed.crop_table: $(BUILD_DIR)/crop_table.exe test_crop_table.sh
	./test_crop_table.sh $< && touch $@

$(BUILD_DIR)/test_passed.shrink_tiles: $(BUILD_DIR)/shrink_tiles.exe test_shrink_tiles.sh
	./test_shrink_tiles.sh $< && touch $@

$(BUILD_DIR)/test_passed.stack_bw: $(BUILD_DIR)/stack_bw.exe test_stack_bw.sh
	./test_stack_bw.sh $< && touch $@

$(BUILD_DIR)/test_passed.generate_unique_tiles: $(BUILD_DIR)/generate_unique_tiles.exe test_generate_unique_tiles.sh
	./test_generate_unique_tiles.sh $< && touch $@

make_build_dir: $(BUILD_DIR)

$(BUILD_DIR):
	mkdir -p $@

clean:
	-rm -rf $(BUILD_DIR)

# }}}
