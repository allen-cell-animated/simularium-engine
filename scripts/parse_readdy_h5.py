import readdy
import numpy as np
import argparse
import json

def main():
    parser = argparse.ArgumentParser(
        description="Parses an hdf5 (*.h5) trajectory file produced\
         by the ReaDDy software and converts it into the Simularium\
         visualization-data-format"
    )
    parser.add_argument("file_path", help="the file path of the trajectory to parse")
    parser.add_argument("output_path", help="the output file path to save the processed trajectory to")
    args = parser.parse_args();

    file_path = args.file_path
    output_path = args.output_path

    traj = readdy.Trajectory(file_path)
    n_particles_per_frame, positions, types, ids = traj.to_numpy(start=0, stop=None)

    # load flavors
    flavors = np.zeros_like(types)
    for typeid in range(np.max(types)):
        if traj.is_topology_particle_type(typeid):
            flavors[types == typeid] = 1
        else:
            flavors[types == typeid] = 0

    # types.shape[1] is the max. number of particles in the simulation
    # for each particle:
    # - vis type 1000 (1)
    # - type id (1)
    # - loc (3)
    # - rot (3)
    # - radius/flavor (1)
    # - n subpoints (1)
    max_n_particles = types.shape[1]
    frame_buf = np.zeros((max_n_particles * 10, ))
    frame_buf[::10] = 1000  # vis type
    data = {}
    data['msgType'] = 1
    data['bundleStart'] = 0
    data['bundleSize'] = len(n_particles_per_frame)
    data['bundleData'] = []
    ix_particles = np.empty((3*max_n_particles,), dtype=int)

    for i in range(max_n_particles):
        ix_particles[3*i:3*i+3] = np.arange(i*10 + 2, i*10 + 2+3)

    for t in range(len(n_particles_per_frame)):
        frame_data = {}
        frame_data['frameNumber'] = t
        frame_data['time'] = float(t)
        n_particles = int(n_particles_per_frame[t])
        local_buf = frame_buf[:10*n_particles]
        local_buf[1::10] = types[t, :n_particles]
        local_buf[ix_particles[:3*n_particles]] = positions[t, :n_particles].flatten()
        local_buf[8::10] = flavors[t, :n_particles]
        frame_data['data'] = local_buf.tolist()
        data['bundleData'].append(frame_data)

    with open(output_path, 'w+') as outfile:
        json.dump(data, outfile)

if __name__ == '__main__':
    main()
