# Shape Rendering & Custom Strokes

To support rich user interfaces, OOEY provides advanced geometric primitives including circles, rounded rectangles, polygons, Bezier curves, and stroked outlines of customizable thickness.

All primitives are compiled down to raw vertex/index buffers (`Geometry`) and drawn using the unified `IRenderTarget::draw_geometry()` API. This composition-based design ensures instant cross-platform portability across X11, Wayland, and Framebuffer backends without modifying the rendering libraries.

---

## 1. Thick Stroke Generation

To render outlines and borders with a stroke thickness $t > 1.0$, the engine converts lines into filled 2D quads (two triangles) oriented along the line's direction:

Given a line segment from start point $A(x_a, y_a)$ to end point $B(x_b, y_b)$:
1. Calculate the direction vector:
   $$\vec{d} = (x_b - x_a, y_b - y_a)$$
2. Normalize it to get $\vec{u}$:
   $$\vec{u} = \frac{\vec{d}}{\|\vec{d}\|}$$
3. Find the perpendicular normal vector $\vec{n}$:
   $$\vec{n} = (-u_y, u_x)$$
4. The four vertices of the quad representing the thick line are:
   $$V_0 = A + \vec{n} \cdot \frac{t}{2}$$
   $$V_1 = A - \vec{n} \cdot \frac{t}{2}$$
   $$V_2 = B - \vec{n} \cdot \frac{t}{2}$$
   $$V_3 = B + \vec{n} \cdot \frac{t}{2}$$
5. Indices for the two triangles are: `0, 1, 2` and `0, 2, 3`.

---

## 2. Advanced Geometric Primitives

### Circle Primitive (`CirclePrimitive`)
A circle is defined by its center point $C(cx, cy)$, radius $r$, fill color, and stroke settings.
- **Fill**: The circle is approximated by $N = 64$ points on its perimeter. The filled area is drawn as a triangle fan from the center $C$ to every pair of adjacent perimeter vertices $P_i$ and $P_{i+1}$:
  $$P_i = (cx + r \cos \theta_i, cy + r \sin \theta_i) \quad \text{where } \theta_i = \frac{2\pi i}{N}$$
- **Stroke**: Drawn as a closed chain of $N$ thick line segments connecting adjacent perimeter vertices.

### Rounded Rectangle (`RoundedRectPrimitive`)
A box with rounded corners is defined by a bounding rectangle `(x, y, w, h)`, corner radius $r$, fill color, and stroke settings.
- **Fill**: The shape is split into 3 filled rectangles and 4 filled corner sectors (quarter-circles):
  - **Horizontal Center**: `(x + r, y, w - 2*r, h)`
  - **Left Side**: `(x, y + r, r, h - 2*r)`
  - **Right Side**: `(x + w - r, y + r, r, h - 2*r)`
  - **Corners**: Quarter-circle fans at Top-Left, Top-Right, Bottom-Right, and Bottom-Left.
- **Stroke**: Formed by 4 straight thick line segments along the edges, joined together by 4 arc segment chains at the corners.

### Convex Polygon (`PolygonPrimitive`)
Defined by an ordered list of vertices $P_0, P_1, \dots, P_{k-1}$.
- **Fill**: Drawn as a triangle fan from $P_0$: triangles $(P_0, P_i, P_{i+1})$ for $1 \le i \le k-2$.
- **Stroke**: Drawn as a closed loop of thick line segments connecting adjacent vertices.

### Bezier Curve (`CurvePrimitive`)
Supports quadratic and cubic Bezier curves defined by a start point $P_0$, end point $P_{end}$, and one or two control points:
- **Quadratic Bezier**:
  $$B(t) = (1-t)^2 P_0 + 2(1-t)t P_{control} + t^2 P_{end}$$
- **Cubic Bezier**:
  $$B(t) = (1-t)^3 P_0 + 3(1-t)^2 t P_{control1} + 3(1-t)t^2 P_{control2} + t^3 P_{end}$$
The curve is discretized into $S = 30$ straight segments. Each segment is rendered as a thick line segment of thickness $t$.
