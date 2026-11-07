# tdapp_demo.py - a small app on the tdapp library: two pages, a gauge
# that redraws itself, and a Modbus RTU slave whose holding register 0 is
# shown by a label (simulated here: no UART is opened).
#
#   py /root/tdapp_demo.py
import tdapp

app = tdapp.App("Oven", 44, 14)
win = app.win

main = app.page("Main")
settings = app.page("Settings")

main.add(win.label(2, 1, "Temperature"))
temperature = main.add(win.label(16, 1, "--"))
cv = main.canvas(2, 3, 24, 8)
dial = tdapp.Field(cv, 0, "gauge", {"x": 0, "y": 0, "w": 48, "h": 16, "value": 0, "maximum": 400})

settings.add(win.label(2, 1, "Set point"))
setpoint = settings.add(win.textbox(14, 1, 8, "250"))


def go_main(button):
    app.show("Main")


def go_settings(button):
    app.show("Settings")


win.button(30, 3, "Main", go_main)
win.button(30, 5, "Settings", go_settings)

link = app.link({"mode": "modbus_slave", "uart": 1, "tx": 17, "rx": 18, "unit": 95, "registerCount": 8},
                simulated=True)
app.bind(temperature, {"table": "hr", "address": 0, "suffix": " C"})

value = [20]


def heat():
    # What a Modbus master would write: the register, then the dial follows.
    value[0] = value[0] + 15 if value[0] < 385 else 20
    link.put("hr", 0, value[0])
    dial.set(value=value[0])


app.every(500, heat)
app.show("Main")
app.run()
