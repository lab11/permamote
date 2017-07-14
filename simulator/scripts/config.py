design_config = {
    'active_period_minutes' : 1,
    'active_time_seconds' : 1E-3,
    'operating_voltage_V' : 3.3,
    'sleep_current_A' : 5E-6,
    'active_current_A' : 20E-3,
    'boost_efficiency' : 0.8,
    'frontend_efficiency' : 1,
    'secondary' : 'lto_battery',
}
primary_config= {
    'capacity_mAh' : 1500,
    'nominal_voltage_V' : 3.6,
    'leakage_percent_month' : 0.083,
}

secondary_lipo_config = {
    'charge_discharge_eff' : 0.95,
    'capacity_mAh' : 50,
    'nominal_voltage_V' : 3.6,
    'lifetime_cycles' : 1000,
    'leakage_percent_month'  : 3,
}

secondary_lto_config = {
    'charge_discharge_eff' : 0.95,
    'capacity_mAh' : 50,
    'nominal_voltage_V' : 2.4,
    'lifetime_cycles' : 10000,
    'leakage_percent_month' : 3,
}

secondary_configs = {
    'lipo_battery' : secondary_lipo_config,
    'lto_battery' : secondary_lto_config,
}

solar_config = {
    'None' : None,
    'nominal_voltage_V' : 2.5,
    'area_cm2' : 10,
    'efficiency' : 0.19,
}
