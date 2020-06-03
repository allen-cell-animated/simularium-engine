import readdy
import sys
import json
import h5py as _h5py

if not len(sys.argv) == 3:
    print("Usage: parse_readdy_h5.py [file_path] [output_file]")
    quit()

file_path = sys.argv[1]
output_path = sys.argv[2]

with _h5py.File(file_path, "r") as f:
    limits = f['readdy']['trajectory']['limits']
    records = f['readdy']['trajectory']['records']
    time = f['readdy']['trajectory']['time']

    data = {}
    data['msgType'] = 1
    data['bundleStart'] = 0
    data['bundleSize'] = len(limits)
    data['bundleData'] = []

    nframes = len(limits)

    for fi in range(0, nframes):
        l = limits[fi]
        t = time[fi]

        frame_data = {}
        frame_data['data'] = []
        frame_data['frameNumber'] = fi
        frame_data['time'] = float(t)

        l_start = int(l[0])
        l_end = int(l[1])

        for ri in range(l_start, l_end):
            if ri >= len(records):
                break

            r = records[ri] # type, guid, radius, [x,y,z]

            # vis type
            frame_data['data'].append(1000)

            # type
            frame_data['data'].append(int(r[0]))

            # location
            frame_data['data'].append(float(r[3][0]))
            frame_data['data'].append(float(r[3][1]))
            frame_data['data'].append(float(r[3][2]))

            # rotation
            frame_data['data'].append(0)
            frame_data['data'].append(0)
            frame_data['data'].append(0)

            # radius
            frame_data['data'].append(float(r[2]))

            # number of sub-points
            frame_data['data'].append(0)

        data['bundleData'].append(frame_data)

with open(output_path, 'w+') as outfile:
    json.dump(data, outfile)
