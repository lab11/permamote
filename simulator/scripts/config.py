
class sim_config:
    def __init__(self):
        self.design_config = {
            'name' : 'design',
            'active_frequency_minutes' : 1,
            'operating_voltage_V' : 3.3,
            'sleep_current_A' : 3E-6,
            'active_current_A' : 4.9E-3,
            'active_period_s': 0.07,
            'boost_efficiency' : 0.8,
            'frontend_efficiency' : 0.8,
            'secondary' : 'lto_battery',
            'secondary_max_percent': 0.22,
            'secondary_min_percent': 0.2,
        }

        self.primary_config= {
            'name' : 'primary',
            'capacity_mAh' : 200,
            'nominal_voltage_V' : 3,
            'leakage_percent_year' : 1,
        }

        self.secondary_lipo_config = {
            'name' : 'secondary',
            'charge_discharge_eff' : 0.95,
            'capacity_mAh' : 50,
            'nominal_voltage_V' : 3.6,
            'lifetime_cycles' : 1000,
            'leakage_constant': 5E4,
        }

        self.secondary_lto_config = {
            'name' : 'secondary',
            'charge_discharge_eff' : 0.95,
            'capacity_mAh' : 50,
            'nominal_voltage_V' : 2.4,
            'lifetime_cycles' : 10000,
            'leakage_constant': 5E4,
        }

        self.secondary_configs = {
            'lipo_battery' : self.secondary_lipo_config,
            'lto_battery' : self.secondary_lto_config,
        }

        self.solar_config = {
            'name' : 'solar',
            'nominal_voltage_V' : 2.5,
            'area_cm2' : 10,
            'efficiency' : 0.19,
        }

        self.config_list = [self.design_config, self.primary_config, self.secondary_configs[self.design_config['secondary']], self.solar_config]

sweep_vars = [
                [('secondary', 'capacity_mAh'), [i*10**exp for exp in range(-5, 2) for i in range(1, 10, 2)]],
                #[('primary', 'capacity_mAh') , 1500, 2500, 100],
                #[('secondary', 'capacity_mAh') , .1, 2, .1]
             ]
