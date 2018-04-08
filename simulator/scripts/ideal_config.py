import math

class config:
    def __init__(self):
        self.battery_type_to_volume_L = {
            #https://en.wikipedia.org/wiki/List_of_battery_sizes
            'AA':   ((14.5 * 1/2)**2 * math.pi * 50.5) * 1E-6,
            'AAA':  ((10.5 * 1/2)**2 * math.pi * 44.5) * 1E-6,
            'CR123A': ((17 * 1/2)**2 * math.pi * 34.5) * 1E-6,
            'CR2032':  ((20 * 1/2)**2 * math.pi * 3.2) * 1E-6,
            'HTC-titanate': ((10 * 1/2)**2 * math.pi * 20) * 1E-6,
        }
        self.chemistry_energy_density_WhpL = {
            # https://en.wikipedia.org/wiki/Comparison_of_commercial_battery_types
            'Alkaline': 320,
            'LiMnO2':   510,
            # https://en.wikipedia.org/wiki/Lithium%E2%80%93titanate_battery
            'titanate': 177,
            'LiPo':     500,
            'LiIon':    475,
        }
        self.design_config = {
            'name' : 'design',
            'intermittent' : True,
            'intermittent_mode' : 'periodic',
            'operating_voltage_V' : 3.3,
            'boost_efficiency' : 0.8,
            'frontend_efficiency' : 0.8,
            'secondary' : 'storage',
        }
        self.secondary_storage_config = {
            'name' : 'secondary',
            'type' : 'capacitor',
            'capacity_J' : 1.9655999999999993,
            'min_capacity_J' : 0,
            'esr_ohm' : 0,
            'leakage_power_W': 0,
        }
        self.secondary_configs = {
            'storage' : self.secondary_storage_config,
        }
        self.solar_config = {
            'name' : 'solar',
            'nominal_voltage_V' : 2.5,
            'area_cm2' : 2.1*4.2,
            'efficiency' : 0.19,
        }
        self.config_list = [self.design_config, self.secondary_configs[self.design_config['secondary']], self.solar_config]

