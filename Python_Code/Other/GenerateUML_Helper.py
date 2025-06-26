# From project at https://github.com/siddharthverma-1607/uml-class-diagram-generator/tree/master
# This generates a xml file. This xml file has each class with the functions (public and private) in boxes in the class
# It does not show the relationship between classes and is just used to speed up the accuracy and efficacy of the UML diagram.

import os
import glob
import ast

def list_python_files(directory):
    return [file for file in glob.glob(os.path.join(directory, '**', '*.py'), recursive=True) if not file.endswith('__init__.py')]

def prompt_user_for_base_package_directory():
    base_directory = input("Enter the name of the base package directory (default to 'src'): ")
    return base_directory if base_directory else 'src'

def prompt_user_for_main_script(files):
    while True:
        print("Select the main script (enter the index, or 'q' to quit):")
        for idx, file in enumerate(files):
            print(f"{idx}: {file}")
        
        user_input = input("Enter index: ")
        if user_input.lower() == 'q':
            print("Exiting program.")
            exit()

        try:
            selected_index = int(user_input)
            if 0 <= selected_index < len(files):
                return files[selected_index]
            else:
                print("Invalid index. Please try again.")
        except ValueError:
            print("Invalid input. Please enter a valid index number.")

def prompt_user_for_package_directory():
    return input("Enter the name of the sub package directory: ")

def extract_imports(file_path):
    try:
        with open(file_path, 'r') as file:
            tree = ast.parse(file.read(), filename=file_path)
    except Exception as e:
        print(f"Error reading or parsing {file_path}: {e}")
        return []
    
    imports = []
    for node in ast.walk(tree):
        if isinstance(node, ast.Import):
            for alias in node.names:
                imports.append(alias.name)
        elif isinstance(node, ast.ImportFrom):
            module = node.module
            for alias in node.names:
                imports.append(f"{module}.{alias.name}")
    
    return imports

def resolve_imported_files(imports, src_directory, package_directory):
    imported_files = []
    for imp in imports:
        if package_directory in imp:
            imp = '.'.join(imp.split(".")[:-1])
            imp_path = os.path.join(src_directory, imp.replace('.', '/') + '.py')
            full_imp_path = os.path.abspath(imp_path)
            if os.path.exists(full_imp_path):
                imported_files.append(full_imp_path)
            else:
                print(f"File not found: {full_imp_path}")
    return imported_files

def parse_class(node):
    class_name = node.name
    methods = []
    attributes = []
    
    for item in node.body:
        if isinstance(item, ast.FunctionDef):
            method_name = item.name
            if method_name.startswith('__') and method_name.endswith('__'):
                methods.append(f"+ {method_name}()")
            elif method_name.startswith('__'):
                methods.append(f"- {method_name}()")
            elif method_name.startswith('_'):
                methods.append(f"# {method_name}()")
            else:
                methods.append(f"+ {method_name}()")
        elif isinstance(item, ast.Assign):
            for target in item.targets:
                if isinstance(target, ast.Name):
                    attr_name = target.id
                    if attr_name.startswith('__'):
                        attributes.append(f"- {attr_name}")
                    elif attr_name.startswith('_'):
                        attributes.append(f"# {attr_name}")
                    else:
                        attributes.append(f"+ {attr_name}")
        elif isinstance(item, ast.AnnAssign):  # Handling annotated assignments
            attr_name = item.target.id
            if attr_name.startswith('__'):
                attributes.append(f"- {attr_name}")
            elif attr_name.startswith('_'):
                attributes.append(f"# {attr_name}")
            else:
                attributes.append(f"+ {attr_name}")
    
    return class_name, methods, attributes

def parse_file(file_path):
    try:
        with open(file_path, 'r') as file:
            tree = ast.parse(file.read(), filename=file_path)
    except Exception as e:
        print(f"Error reading or parsing {file_path}: {e}")
        return []
    
    classes = []
    for node in tree.body:
        if isinstance(node, ast.ClassDef):
            class_info = parse_class(node)
            classes.append(class_info)
    
    return classes

def generate_uml_xml(classes):
    xml_cells = []
    x_position = 100  # Starting X position
    y_position = 100  # Starting Y position
    y_increment = 250  # Increment Y position for each class

    for class_name, methods, attributes in classes:
        attributes_str = "&lt;br/&gt;".join(attributes)  # Ensuring <br/> is correctly placed between attributes
        methods_str = "&lt;br/&gt;".join(methods)  # Ensuring <br/> is correctly placed between methods
        
        cell_value = (f"&lt;p style=&quot;margin:0px;margin-top:4px;text-align:center;&quot;&gt;"
                      f"&lt;b&gt;{class_name}&lt;/b&gt;&lt;/p&gt;&lt;hr size=&quot;1&quot;/&gt;"
                      f"&lt;p style=&quot;margin:0px;margin-left:4px;&quot;&gt;{attributes_str}&lt;/p&gt;&lt;hr size=&quot;1&quot;/&gt;"
                      f"&lt;p style=&quot;margin:0px;margin-left:4px;&quot;&gt;{methods_str}&lt;/p&gt;")
        
        xml_cell = (f'<mxCell id="{class_name}" value="{cell_value}" '
                    f'style="verticalAlign=top;align=left;overflow=fill;fontSize=12;'
                    f'fontFamily=Helvetica;html=1;rounded=0;shadow=0;comic=0;'
                    f'labelBackgroundColor=none;strokeWidth=1" vertex="1" parent="1">'
                    f'<mxGeometry x="{x_position}" y="{y_position}" width="300" height="240" as="geometry" />'
                    f'</mxCell>')
        
        xml_cells.append(xml_cell)
        y_position += y_increment  # Increment Y position for the next class
    
    return "\n".join(xml_cells)

def save_uml_xml_to_file(xml_cells, sub_package_directory):
    file_name = f"{sub_package_directory}_class_diagram.xml"

    xml_content = f'''<?xml version="1.0" encoding="UTF-8"?>
        <mxfile host="app.diagrams.net" agent="Mozilla/5.0" version="24.7.6">
        <diagram name="UML Class Diagram">
            <mxGraphModel dx="1433" dy="922" grid="1" gridSize="10" guides="1" tooltips="1" connect="1" arrows="1" fold="1" page="1" pageScale="1" pageWidth="850" pageHeight="1100" background="none" math="0" shadow="0">
            <root>
                <mxCell id="0" />
                <mxCell id="1" parent="0" />
        {xml_cells}
            </root>
            </mxGraphModel>
        </diagram>
        </mxfile>'''

    try:
        with open(file_name, 'w') as file:
            file.write(xml_content)
        print(f"UML XML saved to {file_name}")
    except Exception as e:
        print(f"Error saving UML XML to file {file_name}: {e}")

def main():
    # Prompt user for base package directory name (default: 'src')
    base_directory = prompt_user_for_base_package_directory()

    # List Python files in the base directory
    python_files = list_python_files(base_directory)
    if not python_files:
        print(f"No Python files found in directory: {base_directory}")
        exit()

    # Prompt user to select the main script
    main_script = prompt_user_for_main_script(python_files)

    # Prompt user for sub-package directory name
    sub_package_directory = prompt_user_for_package_directory()

    # Extract imports from the main script
    imports = extract_imports(main_script)

    # Resolve imported files based on package directory
    imported_files = resolve_imported_files(imports, base_directory, sub_package_directory)
    imported_files.append(main_script)

    # Parse files and generate UML XML
    all_classes = []
    for file_path in imported_files:
        classes = parse_file(file_path)
        all_classes.extend(classes)

    uml_xml = generate_uml_xml(all_classes)

    # Save UML XML to file
    save_uml_xml_to_file(uml_xml, sub_package_directory)

if __name__ == "__main__":
    main()
