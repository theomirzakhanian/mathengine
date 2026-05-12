# MathEngine

A desktop math engine I built in C++17. Started as a graphing calculator and kept growing. It now has a parser, AST, bytecode evaluator, a real CAS, 3D surfaces, matrix tools, data fitting, and a drawing tool that turns your strokes into Desmos equations.

No external math libraries. The engine is its own thing.

## What's in it

Nine tabs.

**Graph** is the obvious one. Multi-plot, cartesian/polar/parametric, auto-generated parameter sliders if your expression has letters other than x. Integral shading, crosshair readouts, themes, PNG export.

**Calculator** handles the symbolic stuff: simplify, expand, factor, derivatives, integrals, Taylor series, limits. The equation solver does exact roots for polynomials up to degree 4 and linear systems of any size.

**3D** plots `z = f(x,y)`. Drag to orbit the camera. Wireframe, filled shading colored by height, contour lines.

**Matrix** has two editable grids and one-click ops: determinant, inverse, transpose, eigenvalues, LU, QR, solve `Ax = b`.

**Data Lab** takes CSV. Paste it, pick a polynomial degree, you get a regression curve over the scatter plot with R² shown. Copy to Desmos.

**Implicit** renders things like `x² + y² = 9` using marching squares. Inequality shading works too, so `y < x²` fills the region underneath.

**Design** is the weird one. You draw shapes freehand. Each stroke gets fitted as a parametric polynomial. You can load a background image and trace over it. Export to Desmos and the strokes come out as parametric equations that draw the same picture.

**Tools** is a grab bag: constants, units, primes, statistics, complex numbers, polynomial root finder.

**Console** is a REPL. Variables, function defs, lookup tables, curve fitting. `help` lists everything.

## Build

CMake 3.20+ and a C++17 compiler. Dependencies fetch automatically the first time.

```bash
cmake -B build
cmake --build build -j
./build/app/mathapp
```

Tests:

```bash
./build/tests/math_tests
```

90-ish test cases, 200+ assertions. The Desmos exporter has its own roundtrip suite that parses the exported LaTeX back and confirms the values match.

## Under the hood

`libmathengine` is a static library with zero GUI deps, so the engine is reusable on its own. The AST is a `std::variant` for cache friendliness. Parser is recursive-descent. On top of the AST there's a bytecode evaluator that the graph uses for the hot path (sampling thousands of points per frame).

GUI is GLFW + OpenGL 3.3 + Dear ImGui (docking branch). Dependencies come through CMake FetchContent. The stb headers are vendored.

```
engine/         libmathengine
app/            Desktop app
tests/          Catch2 unit tests
third_party/    stb_image, stb_image_write
```

## Desmos export notes

The hard part of exporting to Desmos isn't generating LaTeX, it's making sure Desmos parses it the same way the engine does. A few things bit me hard. Scientific notation is the big one — Desmos reads `1e-5` as `1 * e^(-5)` (Euler's number) and silently mangles your function. The exporter only emits fixed-point numbers. There's also a unary-minus precedence trap where Desmos reads `-x^2` as `-(x^2)` but most parsers including this one read it as `(-x)^2`. The verifier suite catches both.

## License

MIT
