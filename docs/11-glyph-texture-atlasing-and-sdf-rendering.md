# Glyph Texture Atlasing & SDF Font Rendering Methodology

This document details the architectural methodology, mathematical distance algorithms, shelf packing metrics, and GPU/CPU quad streaming pipelines behind the **Glyph Texture Atlas** and **Signed Distance Field (SDF)** rendering engine.

---

## 1. Architectural Challenge & Design Methodology

### The Bottleneck: Active-Frame Grayscale Rasterization
Prior to this implementation, the OOEY engine rendered text on the fly:
- For every frame, the scene graph's `TextPrimitive` called the render target's `draw_text()`.
- The render target queried the platform-specific font backend (FreeType on Linux, GDI on Windows).
- The font backend traversed every character of the string, loaded the glyph outline from the font file, rasterized it to a grayscale alpha map, and fired a pixel-by-pixel callback loop.
- In drawing a 100-row process telemetry grid, the CPU executed millions of coordinate conversions, pointer callbacks, and draw instructions per second. This caused cache thrashing, CPU load spikes, and heap allocation pressure.

### The Solution: Decoupled Pre-Rasterized Atlasing
We decoupled the font system from the graphics pipeline using a hybrid texture-caching scheme:
1. **Load-Time Pre-Rasterization**: When a font configuration is matched or loaded, ASCII printable characters (`[32, 127)`) are pre-rasterized *once*.
2. **Device-Independent GlyphAtlas**: Characters are packed into a single 512x512 memory-resident `Image` buffer. Coordinate maps and advance metrics are cached on the CPU.
3. **Streamlined Pipeline**: Rendering text is reduced to fetching pre-packed quads and mapping their UV coordinates, eliminating the runtime FreeType rasterization loop.

---

## 2. Shelf Packing Layout Algorithm

To fit 95 characters of varying dimensions into a single 512x512 buffer, we implemented a deterministic **shelf-packing** algorithm.

Let:
- $W_{\text{atlas}}, H_{\text{atlas}}$ be the width and height of the master atlas image (512x512 pixels).
- $x, y$ be the current cursor position in the packer, initialized to $(2, 2)$ to avoid border overlap.
- $h_{\text{shelf}}$ be the height of the current row (shelf), initialized to 0.
- $w_{\text{glyph}}, h_{\text{glyph}}$ be the dimensions of the character glyph (including distance field padding).

For each character $C \in [32, 127)$:
1. If the column space is exhausted:
   $$x + w_{\text{glyph}} + 2 > W_{\text{atlas}}$$
   We wrap to a new shelf:
   $$x = 2$$
   $$y = y + h_{\text{shelf}} + 2$$
   $$h_{\text{shelf}} = 0$$
2. If the row space is exhausted ($y + h_{\text{glyph}} > H_{\text{atlas}}$), a warning is printed and generation halts.
3. The glyph is copied to the atlas at coordinates $(x, y)$.
4. The tracking metrics are updated:
   $$h_{\text{shelf}} = \max\left(h_{\text{shelf}}, h_{\text{glyph}}\right)$$
   $$x = x + w_{\text{glyph}} + 2$$

---

## 3. Signed Distance Field (SDF) Mathematics

Standard character bitmaps blur or exhibit blocky aliasing when scaled. A Signed Distance Field (SDF) stores the Euclidean distance to the nearest glyph edge at each pixel, allowing sharp, anti-aliased scaling.

### SDF Calculation
For a raw glyph bitmap $G$ with bounding box dimensions $W \times H$, we add padding $P = 4$ pixels on all sides to allow the distance field to project outside the shape boundaries. The padded buffer size is $W_{\text{pad}} = W + 2P$ and $H_{\text{pad}} = H + 2P$.

For each pixel $(x, y) \in [0, W_{\text{pad}}) \times [0, H_{\text{pad}})$:
1. Translate to raw unpadded coordinates:
   $$ux = x - P,\quad uy = y - P$$
2. Determine if the coordinate is inside the glyph:
   $$\text{Inside}(ux, uy) \iff (ux \in [0, W) \land uy \in [0, H) \land G(ux, uy) > 127)$$
3. Search the neighborhood for the closest pixel $(ox, oy)$ of the opposite state ($\text{Inside}(ux, uy) \neq \text{Inside}(ox, oy)$):
   $$d = \min_{\text{Inside}(ox, oy) \neq \text{Inside}(ux, uy)} \sqrt{(ux - ox)^2 + (uy - oy)^2}$$
4. Compute the signed distance $S$:
   $$S(ux, uy) = \begin{cases} d & \text{if Inside}(ux, uy) \\ -d & \text{otherwise} \end{cases}$$
5. Normalize the distance value to fit within a `uint8_t` byte:
   $$\text{SdfPixel}(x, y) = \text{clamp}\left(128.0 + S(ux, uy) \times \frac{128.0}{P}, 0.0, 255.0\right)$$

The edge of the glyph is located at the threshold value of **128**.

```
    Inside Shape (S > 0)          Edge (S = 0)         Outside Shape (S < 0)
    +---------------------------+--------------+-----------------------------+
    | 255 . . . . . . . . . 129 |     128      | 127 . . . . . . . . . . . 0 |
    +---------------------------+--------------+-----------------------------+
```

---

## 4. Quad Batching & Render Target Pipelines

By shifting to the pre-rendered atlas, we refactored each target to optimize rendering:

### 1. OpenGL (`GlRenderTarget`)
- Binds the 512x512 texture once.
- Loops through the string characters, querying `GlyphMetrics`.
- Coordinates are mapped using the normalized texture UV coordinates:
  $$u_0 = \frac{x_{\text{atlas}}}{W_{\text{atlas}}},\quad v_0 = \frac{y_{\text{atlas}}}{H_{\text{atlas}}}$$
  $$u_1 = \frac{x_{\text{atlas}} + w_{\text{glyph}}}{W_{\text{atlas}}},\quad v_1 = \frac{y_{\text{atlas}} + h_{\text{glyph}}}{H_{\text{atlas}}}$$
- Emits quads inside a single OpenGL immediate mode block:
  ```cpp
  glBegin(GL_QUADS);
  for (char c : text) {
      // glTexCoord2f(u, v); glVertex2f(x, y);
  }
  glEnd();
  ```

### 2. Software Rasterizer (`SoftwareRenderTarget`)
- For each character, the sub-rectangle bounds are mapped to the software framebuffer.
- To achieve smooth anti-aliased edges, we apply a smooth-step filter on the sampled distance values:
  $$\text{Edge} = 128,\quad\text{Width} = 8$$
  $$\text{Alpha}_{\text{factor}} = \text{clamp}\left(\frac{\text{SdfValue} - (\text{Edge} - \text{Width})}{2 \times \text{Width}}, 0.0, 1.0\right)$$
  $$\text{Alpha}_{\text{blended}} = \text{Alpha}_{\text{factor}} \times \text{Color}_{\text{alpha}}$$
- Blits the pixels using the fast, division-free blending multiplier `div255`.

### 3. Vulkan (`VulkanRenderTarget`)
- Maps the layout coordinates directly from the pre-cached atlas.
- Appends character vertices and index buffers directly into the frame arrays, triggering one batch draw call instead of rendering thousands of 1x1 quads.
- Operates at high speeds since CPU-GPU memory pipeline synchronization overhead is removed.

---

## 5. Sharpness & Alignment Correction: Transition to GL_NEAREST

While standard images benefit from bilinear scaling (`GL_LINEAR`), rendering a font atlas requires exact 1:1 mapping on the pixel grid. 

### Why GL_LINEAR Fails on Font Atlases
1. **Edge Bleeding:** bilinear filtering interpolates across glyph margins, blending transparent padding with glyph boundaries, which creates a blurry halo around characters.
2. **Subpixel Shifts:** If the viewport coordinates mapping maps texel borders directly onto pixel edges, linear filtering averages adjacent texels, causing a 0.5-pixel visual offset and fuzzy lines.
3. **Double Anti-aliasing:** The characters are already rasterized with high-quality anti-aliasing. Adding linear interpolation on top introduces unnecessary blur.

### The Fix
To ensure text is crisp and aligned, the OpenGL backend maps the textures using nearest-neighbor filtering:
```cpp
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
```
This forces the graphics card to sample exactly the pre-rasterized anti-aliased texel centers, yielding pixel-perfect, sharp text across all platforms.

