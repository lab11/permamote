import math

class config:
    def __init__(self):
        self.design_config = {
            'name' : 'design',
            'intermittent' : True,
            'intermittent_mode' : 'opportunistic', # periodic or opportunistic
            'operating_voltage_V' : 3.3,
            'boost_efficiency' : 0.8,
            'frontend_efficiency' : 0.8,
            'secondary' : 'cap',
        }
        self.secondary_cap = {
            'name' : 'secondary',
            'type' : 'capacitor',
            'esr_ohm' : 0.1,
            'capacity_J': .5*500E-6*(2.4**2),
            'min_capacity_J': .5*500E-6 * (0.6**2),
            'leakage_power_W': 10E-9 * 3,
        }
        self.secondary_configs = {
            'cap' : self.secondary_cap,
        }
        self.solar_config = {
            'name' : 'solar',
            'nominal_voltage_V' : 2.2,
            'area_cm2' : 4.86,
            'efficiency' : 0.17,
        }
        self.config_list = [self.design_config, self.secondary_configs[self.design_config['secondary']], self.solar_config]

