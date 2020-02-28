# VISUALIZATION DATA FORMAT
Trajectory frames are sent from the Agent-Viz back-end in a JSON format documented below.

## Frame Bundle Encoding
Trajectory frames are broadcast in bundles; each frame has a format that it is encoded into, and each bundle has an array of these objects, and some information about the bundle.

### Example
A bundle with 5 frames starting at frame number 0:
```
{
    msgType:1,
    bundleStart: 0,
    bundleSize: 5,
    bundleData: [
        { frameNumber: 0, time: 0, data: [...] },
        { frameNumber: 1, time: 5, data: [...] },
        { frameNumber: 2, time: 10, data: [...] },
        { frameNumber: 3, time: 15, data: [...] },
        { frameNumber: 4, time: 20, data: [...] }
    ]
}
```

### Fields
#### bundleStart
The first frame contained in the bundle. Above, this is frame number 0.

#### bundleSize
How many individual frames are contained in the data bundle. Above, 5 frames are included (frames 0 - 4).

#### bundleData
An array containing the individual data frames encoded in the format described below.

## Individual Frame Encoding
Individual frames are encoded into a more compact format before being packed into the
bundleData field mentioned above.

### Example
A single frame at time 200 with a single agent:
```
{  
    "data" : [  
        1000.0,  
        2.0,  
        15.5,
        15.6,
        15.7,  
        45.25,  
        45.26,  
        45.27,  
        1.0,  
        0.0  
    ],  
    "frameNumber" : 1,  
    "msgType" : 1  
    "time" : 200  
}  
```

The corresponding object constructed in Agent-Viz (C++):

```
using namespace aics::agentsim;

AgentData ad;
ad.vis_type = kVisType::vis_type_default; // 1000
ad.type = 2;
ad.x = 15.5;
ad.y = 15.6;
ad.z = 15.7;
ad.xrot = 45.25;
ad.yrot = 45.26;
ad.zrot = 45.27;
ad.cr = 1.0
ad.subpoints = std::vector<float>(); // empty, included for clarity
```

## Agent Data
Data on the back-end is represented using the AgentData structure. This structure contains basic spatial information about the agent as well as information on how to visualize the agent.

### Fields
The fields of the AgentData structure are streamed in the following order:

* vis_type
* type
* x
* y
* z
* xrot
* yrot
* zrot
* collision_radius
* subpoints

#### vis_type

* **1000** : `vis_type_default` the default option, renders the object using a defined mesh
* **1001** : `vis_type_fiber` use the sub-points field to draw the agent as a fiber

#### type
An integer value describing the agents type; agents with a different type will be rendered with different colors.

#### x, y, z
The three components of the agent's spatial position.

#### xrot, yrot, zrot
The three components of the agent's spatial rotation.

#### collision_radius
If the agent is visualized as a `vis_type_fiber`, this is used to determine the thickness of the fiber visualized.

#### subpoints
If the agent is visualized as a `vis_type_fiber`, these are used to render a fiber. This array is variable length, with the expectation that future visualization options may use the contained information differently.

## Outgoing Data
The outgoing JSON has the following fields:

* data
* frameNumber
* msgType
* time

Values are written to the data field in the following order: **vis_type | type | x | y | z | xrot | yrot | zrot | collision_radius | size of subpoints | N subpoint-values**

If there are the following **six** subpoints (0,0,0,1,1,1), then the following will be written after the non-variable-length values: (**6**,0,0,0,1,1,1); the front-end will render a fiber going through the points (0,0,0) and (1,1,1).
