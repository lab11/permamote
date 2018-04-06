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
            'intermittent_mode' : 'periodic', # periodic or opportunistic
            'operating_voltage_V' : 3.3,
            'boost_efficiency' : 0.8,
            'frontend_efficiency' : 0.8,
            'secondary' : 'extra_super_cap',
        }
        self.secondary_cap = {
            'name' : 'secondary',
            'type' : 'capacitor',
            'esr_ohm' : 0.1,
            'capacity_J': 1.292E-4 * (3.3**2),
            'min_capacity_J': 1.292E-4 * (0.4**2),
            'leakage_power_W': 10E-9 * 3,
        }
        self.secondary_super_cap = {
            'name' : 'secondary',
            'type' : 'capacitor',
            'esr_ohm' : 25,
            'capacity_J': (300E-6 + 1100E-6 + 7.5E-3) * (3.3**2),
            'min_capacity_J': (1000E-6 + 7.5E-3) * (0.4**2),
            'leakage_power_W': 10E-9 * 2.4,
        }
        self.secondary_extra_super_cap = {
            'name' : 'secondary',
            'type' : 'capacitor',
            'esr_ohm' : 200,
            'capacity_J': 100E-3  * (3.3**2),
            'min_capacity_J': 100E-3  * (0.4**2),
            'leakage_power_W': 10E-9 * 2.4,
        }
        self.secondary_configs = {
            'cap' : self.secondary_cap,
            'super_cap' : self.secondary_super_cap,
            'extra_super_cap' : self.secondary_extra_super_cap,
        }
        self.solar_config = {
            'name' : 'solar',
            'nominal_voltage_V' : 2.5,
            'area_cm2' : 2.1*4.2,
            'efficiency' : 0.19,
        }
        self.config_list = [self.design_config, self.secondary_configs[self.design_config['secondary']], self.solar_config]

