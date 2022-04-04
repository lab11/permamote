#! /usr/bin/env python3
import multiprocessing
import yaml
from simulate import EHSim
from pprint import pprint

def simulate(sim):
    return sim.simulate()

if __name__ == "__main__":
    pool = multiprocessing.Pool(processes = 20)

    with open('design_configs/ideal_config_primary.yaml', 'r') as c_file, \
         open('workload_configs/sense_and_send.yaml', 'r') as wl_file, \
         open('dataset_configs/setupa_dataset.yaml', 'r') as d_file:
        try:
            config = yaml.safe_load(c_file)
            workload = yaml.safe_load(wl_file)['workload']
            dataset = yaml.safe_load(d_file)['dataset']
        except yaml.YAMLError as e:
            print(e)
            exit(1)
    simulator1 = EHSim(config, workload, dataset)
    pprint(vars(simulator1))

    config['design_config']['using_secondary'] = True
    config['design_config']['using_harvester'] = True
    simulator2 = EHSim(config, workload, dataset)
    pprint(vars(simulator2))

    simulators = [simulator1, simulator2]

    results = pool.map(simulate, simulators)
    for result in results:
        pprint(result)
        print('percent energy utilized: \t{0:.2f}%'.format(100 * result['fraction_energy_utilized']))
        print('percent online: \t\t{0:.2f}%'.format(100 * result['time_online']))
        print('percent successful events: \t{0:.2f}%'.format(100 * result['events_success'] / (result['events_success'] + result['events_missed'])))
        print('average ttc: \t{0:.2f}s'.format(result['event_ttc_mean']))

