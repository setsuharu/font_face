# font_face
Simple command line utility to extract glyphs from a truetype/opentype font file as bitmaps.

Getting a glyph bitmap looks like this:
<pre><code>
   fontface -f arial.ttf -u 65 -p 64
</code></pre>
This will get 'A' from 'arial.ttf' at '64pt' size. (DPI value of 300 is used in calculations of glyph outline)
Use -h for help.

Here are some examples using the 'GrisaiaCustom.ttf' font file (rendered at 64pt):
![fontface output example](https://files.catbox.moe/thgk7l.png)
Anti-aliasing is not implemented.

Special Thanks
--------------
- https://docs.microsoft.com/en-us/typography/opentype/spec/
- https://opentype.js.org/
- https://stuff.mit.edu/afs/athena/astaff/source/src-9.0/third/freetype/docs/glyphs/glyphs-6.html