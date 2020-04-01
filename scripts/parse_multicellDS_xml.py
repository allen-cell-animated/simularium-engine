import os
import numpy as np
from pathlib import Path
from pyMCDS import pyMCDS
import json


model_name = "3D_cancer_example"
path_to_xml_files = "../../PhysiCell/output/" 
json_output_path = "output/" 

# ---------------------------------------------------------------------------------------------------------

'''
load simulation data from PhysiCell MultiCellDS XML files
'''
def load_simulation_data():
    
    sorted_files = sorted(Path(path_to_xml_files).glob('output*.xml'))
    data = []
    for file in sorted_files:
        data.append(pyMCDS(file.name, False, path_to_xml_files))
        
    return np.array(data)


ids = {}
last_id = 0

'''
get a unique agent type id for a specific cell type and phase combination
'''
def get_agent_type(cell_type, cell_phase):
    
    global ids, last_id
    
    if cell_type not in ids:
        ids[cell_type] = {}
        
    if cell_phase not in ids[cell_type]:
        print("ID {} : cell type = {}, cell phase = {}".format(last_id, cell_type, cell_phase))
        ids[cell_type][cell_phase] = last_id
        last_id += 1
    
    return ids[cell_type][cell_phase]


'''
get data from one time step in Simularium format
'''
def get_visualization_data_one_frame(index, sim_data_one_frame):
    
    discrete_cells = sim_data_one_frame.get_cell_df()
    
    data = {}
    data['data'] = []
    data['frameNumber'] = index
    data['time'] = sim_data_one_frame.data['metadata']['current_time']
    
    for i in range(len(discrete_cells['position_x'])):
        
        if i >= 500:
            return data
        
        data['data'].append(1000.0)                                                               # viz type = sphere
        data['data'].append(float(get_agent_type(int(discrete_cells['cell_type'][i]),             # agent type 
                                                 int(discrete_cells['current_phase'][i]))))                                                      
        data['data'].append(round(discrete_cells['position_x'][i], 1))                            # X position
        data['data'].append(round(discrete_cells['position_y'][i], 1))                            # Y position
        data['data'].append(round(discrete_cells['position_z'][i], 1))                            # Z position
        data['data'].append(round(discrete_cells['orientation_x'][i], 1))                         # X orientation 
        data['data'].append(round(discrete_cells['orientation_x'][i], 1))                         # Y orientation
        data['data'].append(round(discrete_cells['orientation_x'][i], 1))                         # Z orientation
        data['data'].append(round(np.cbrt(3./4. * discrete_cells['total_volume'][i] / np.pi), 1)) # scale 
        data['data'].append(0.0)                                                                  # ?
    
    return data


'''
get data in Simularium format
'''
def get_visualization_data(sim_data):
    
    data = {}
    data['msgType'] = 1
    data['bundleStart'] = 0
    data['bundleSize'] = len(sim_data)
    data['bundleData'] = []
    
    for i in range(len(sim_data)):
        data['bundleData'].append(get_visualization_data_one_frame(i, sim_data[i]))
        
    return data


'''
write all data to a json file
'''
def write_json_data(viz_data):
    
    if not os.path.exists(json_output_path):
        os.makedirs(json_output_path)
        
    with open('{}{}.json'.format(json_output_path, model_name), 'w+') as outfile:
        json.dump(viz_data, outfile)


'''
convert a set of MultiCellDS XML files to a Simularium JSON file
'''
def convert_xml_to_json():
    
    write_json_data(get_visualization_data(load_simulation_data()))
    
    print("wrote Simularium JSON with {} agent types".format(last_id))


# ---------------------------------------------------------------------------------------------------------
        
convert_xml_to_json()
    