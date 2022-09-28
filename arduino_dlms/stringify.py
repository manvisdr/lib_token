Import("env")

env.Append(CPPDEFINES=[
    ("SSID", env.StringifyMacro('"MEL_MonKWh_0002"')),
])
