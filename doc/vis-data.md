# Vis Data Format
A JSON File containing geometry information for a given trajectory.

## Format
The vis-data file maps numerical ids to geometry information - scale, and a geometry file to load. Currently, both the vis-data file and the geometry referred to are stored on S3. Currently, this is created manually. The eventual goal is for this to be generated when a user creates a model through the Simularium front-end.

### Naming
A corresponding vis-data file for `Trajectory.bin` is expected to be named `Trajectory.bin.json`. This is to make programmatic access easier. This file should be located at `[S3-Location]/visdata/Trajectory.bin.json`.

### Example
```
{
    "0" : {
        "mesh": "proton.obj",
        "scale": 0.1
    },
    ...
    "100" : {
        "mesh": "membrane.obj",
        "scale": 1.0
    }
}
```

In the above example, for an agent with ID 1, the geometry from `[S3-Location]/meshes/proton.obj` will be used for rendering. The geometry in the viewer will be scaled by 0.1 relative to the geometry in the proton.obj file.
