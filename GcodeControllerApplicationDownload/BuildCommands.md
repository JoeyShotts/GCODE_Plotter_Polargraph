The Application was built into an exe using pyinstaller.
pyinstaller --onefile --noconsole --add-binary ".\\GeneratePoints.exe;." --icon ".\\Plotter_Icon.ico" --add-data ".\\Plotter_Icon.ico;." --name GCODE_Plotter  main.py
