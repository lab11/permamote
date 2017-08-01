
class sim_config:
    design_config = {
        'name' : 'design',
        'active_frequency_minutes' : 1,
        'payload_size_bytes' : 64,
        'radio_bandwidth_kbps': 250,
        'operating_voltage_V' : 3.3,
        'sleep_current_A' : 10E-6,
        'active_current_A' : 20E-3,
        'boost_efficiency' : 0.8,
        'frontend_efficiency' : 0.8,
        'secondary' : 'lto_battery',
        'secondary_max_percent': .35,
        'secondary_min_percent': .3,
    }

    primary_config= {
        'name' : 'primary',
        'capacity_mAh' : 1500,
        'nominal_voltage_V' : 3.6,
        'leakage_percent_month' : 0.083,
    }

    secondary_lipo_config = {
        'name' : 'secondary',
        'charge_discharge_eff' : 0.95,
        'capacity_mAh' : 50,
        'nominal_voltage_V' : 3.6,
        'lifetime_cycles' : 1000,
        'leakage_constant': 5E4,
    }

    secondary_lto_config = {
        'name' : 'secondary',
        'charge_discharge_eff' : 0.95,
        'capacity_mAh' : 8,
        'nominal_voltage_V' : 2.4,
        'lifetime_cycles' : 10000,
        'leakage_constant': 5E4,
    }

    secondary_configs = {
        'lipo_battery' : secondary_lipo_config,
        'lto_battery' : secondary_lto_config,
    }

    solar_config = {
        'name' : 'solar',
        'nominal_voltage_V' : 2.5,
        'area_cm2' : 10,
        'efficiency' : 0.19,
    }

    config_list = [design_config, primary_config, secondary_configs[design_config['secondary']], solar_config]

sweep_vars = [
                [('solar', 'area_cm2') , 1, 10, .5],
                #[('secondary', 'capacity_mAh') , 1, 50, 10]
             ]
