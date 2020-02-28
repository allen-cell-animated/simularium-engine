# PROCESSED TRAJECTORIES
Simulation trajectories often contain a great deal of data about relations important for specific simulation systems. Simularium only broadcasts a subset of that information to web-front-ends for visualization, including spatial transforms and identifying information. In addition, Simularium broadcasts information about how to render specific entities (e.g. fibers).

To reduce processing time, intermediate processed results are saved in a binary format to S3. These intermediate files are described below, and contain information in a compact format read by the `SimulationCache` object. All trajectory files & processed results are currently stored under `[S3-Location]/trajectory`.

## Files
Two files are stored for each processed trajectory file. One is a JSON file with meta-data about the file and the other is a binary file containing trajectory data that that can be easily encoded to the visualization-data-format.

### The Info File
#### Example
```
{
    "boxSizeX": 200,
    "boxSizeY": 200,
    "boxSizeZ": 200,
    "fileName": "Polymer_Network.bin",
    "numberOfFrames": 2001,
    "timeStepSize": 20.0,
    "typeMapping" : {
        "1" : "Monomer",
        "2" : "Custom Name",
        "3" : "Dimer",
        "4" : "Trimer",
        "5" : "Polymer"
    }
}
```
### The Binary Cache
There is only one point of access for these files, the `SimulationCache` object. This is by design. From the `Simulation` object upwards, the experience of streaming a file from a pre-processed cache and processing the file on demand should be identical. Only the `SimulationCache` should be writing or reading these cached binaries.

#### Example
```
F0/96/-89.355/-26.0234/2.34483/0/0/0/1/1000/0/...
F1/96/-87.2896/-26.2228/3.32163/0/0/0/1/1000/0/...
....
```
Agents are serialized in the following order:
* x
* y
* z
* xrot
* yrot
* zrot
* vis_type
* type
* collision_radius
* n-subpoints
* subpoints [...]

See `visualization-data-format.md` for more information about these fields.
