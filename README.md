# font_face
To use in your project, put 'bitmap', 'util', and 'font_face' hpp and cpp files in a folder together and include 'font_face.hpp' where needed.
The texture atlas class is not required and was only used to help with examining the results of the glyph rasterizer.

Getting a glyph bitmap looks like this:
<pre><code>
    tou::font_face face("arial.ttf");
    uint16_t ch = 0x0042; // letter 'B'
    float pointsize = 64.0f;
    auto glyph = face.get_glyph_bitmap(ch, pointsize, true, true);
    makefile("glyph.bmp", glyph.image);
</code></pre>

makefile can be defined as:
<pre><code>
    void makefile(const std::string& filename, const tou::bitmap_image& bmp)
    {
	    std::vector<char> v;
	    bmp.file(v);
	    std::ofstream out;
	    out.open(filename, std::ios::binary | std::ios::out);
	    out.write(v.data(), v.size());
	    out.close();
    }
</code></pre>

Known Issues
--------------
In the rasterizer function, some small regions of a glyph may end up unfilled. This happens because the fill operation expects the calculated outline to be 'nice' (no isolated jumps in a segment of an outline) but an outline may not always be 'nice' especially at point sizes less than 16.

Misc
--------------
- Contributions are welcome
- You may use the code however you please. If you do, a link to my github page would be appreciated but is not required.

Special Thanks
--------------
- https://docs.microsoft.com/en-us/typography/opentype/spec/
- https://opentype.js.org/
- https://stuff.mit.edu/afs/athena/astaff/source/src-9.0/third/freetype/docs/glyphs/glyphs-6.html