## Animation Module

### Planning

- one big pool for all animation keys
- keys -> curves -> clips -> libraries
- keys are 1D floats, the animation system doesn't know about points, vectors, quaternions, colors, etc...
- this means that quaternions are *not* spherical interpolated, so rotation keys must be near each other
- ...if this turns out to be a problem, rotation keys must be somehow identified  (though a ClipLayout?)
- a curve is a 'column of keys'
- a clip is a 'row of curves', or a 2D table of keys
- a library is a collection of clips with the same number of curves:
- a slice is the group of keys at the same point in time

```
an Anim Library with 3 clips and 10 curves:
+===+===+===+===+===+===+===+===+===+===+ Clip 0
|   |   |   |   |   |   |   |   |   |   | -> a slice of keys 
+---+---+---+---+---+---+---+---+---+---+    at time 0
|   |   |   |   |   |   |   |   |   |   |
+---+---+---+---+---+---+---+---+---+---+
|   |   |   |   |   |   |   |   |   |   |
+---+---+---+---+---+---+---+---+---+---+
|   |   |   |   |   |   |   |   |   |   |
+---+---+---+---+---+---+---+---+---+---+
|   |   |   |   |   |   |   |   |   |   |
+===+===+===+===+===+===+===+===+===+===+ Clip 1
|   |   |   |   |   |   |   |   |   |   |
+---+---+---+---+---+---+---+---+---+---+
|   |   |   |   |   |   |   |   |   |   |
+---+---+---+---+---+---+---+---+---+---+
|   |   |   |   |   |   |   |   |   |   |
+---+---+---+---+---+---+---+---+---+---+
|   |   |   |   |   |   |   |   |   |   |
+===+===+===+===+===+===+===+===+===+===+ Clip 2
|   |   |   |   |   |   |   |   |   |   |
+---+---+---+---+---+---+---+---+---+---+
|   |   |   |   |   |   |   |   |   |   |
+---+---+---+---+---+---+---+---+---+---+
|   |   |   |   |   |   |   |   |   |   |
+---+---+---+---+---+---+---+---+---+---+
|   |   |   |   |   |   |   |   |   |   |
+---+---+---+---+---+---+---+---+---+---+
|   |   |   |   |   |   |   |   |   |   |
+---+---+---+---+---+---+---+---+---+---+
|   |   |   |   |   |   |   |   |   |   |
+---+---+---+---+---+---+---+---+---+---+
|   |   |   |   |   |   |   |   |   |   |
+===+===+===+===+===+===+===+===+===+===+
```

Hmm, what about curves that are not actually animated... these 
are quite common (often only the rotation in a character skeleton
is animated for instance...)

