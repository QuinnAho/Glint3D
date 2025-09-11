#!/usr/bin/env python3
"""
Glint3D Intent Code Generator

Generates boilerplate code for new intents based on schema definitions.
This ensures consistency and reduces manual coding errors when adding
new functionality.

Usage:
    python tools/codegen/intent_generator.py --intent add_material --schema schemas/json_ops_v2.json
"""

import json
import argparse
from pathlib import Path
from typing import Dict, Any

class IntentGenerator:
    def __init__(self, schema_path: Path):
        self.schema_path = schema_path
        self.schema = self.load_schema()
        
    def load_schema(self) -> Dict[str, Any]:
        """Load JSON schema file"""
        with open(self.schema_path) as f:
            return json.load(f)
    
    def generate_sdk_method(self, intent_name: str, params: Dict[str, Any]) -> str:
        """Generate TypeScript SDK method for an intent"""
        method_name = self.intent_to_method_name(intent_name)
        
        # Generate parameter interface
        param_interface = self.generate_param_interface(intent_name, params)
        
        # Generate method signature
        method_sig = f"async {method_name}(params: {intent_name}Params): Promise<any>"
        
        # Generate method body
        method_body = f'''  {method_sig} {{
    return this.executeIntent('{intent_name}', params);
  }}'''
        
        return f"{param_interface}\n\n{method_body}"
    
    def generate_param_interface(self, intent_name: str, params: Dict[str, Any]) -> str:
        """Generate TypeScript interface for intent parameters"""
        interface_name = f"{intent_name}Params"
        
        properties = []
        for param_name, param_info in params.items():
            ts_type = self.json_type_to_ts_type(param_info)
            optional = "?" if not param_info.get('required', False) else ""
            properties.append(f"  {param_name}{optional}: {ts_type};")
        
        props_str = "\n".join(properties)
        
        return f'''interface {interface_name} {{
{props_str}
}}'''
    
    def generate_cpp_handler(self, intent_name: str, params: Dict[str, Any]) -> str:
        """Generate C++ handler function for an intent"""
        function_name = f"handle_{intent_name}"
        
        # Generate parameter parsing
        param_parsing = []
        for param_name, param_info in params.items():
            cpp_type = self.json_type_to_cpp_type(param_info)
            param_parsing.append(f"    {cpp_type} {param_name} = params[\"{param_name}\"];")
        
        parsing_str = "\n".join(param_parsing)
        
        return f'''nlohmann::json {function_name}(const nlohmann::json& params) {{
    // Parse parameters
{parsing_str}
    
    // TODO: Implement {intent_name} logic
    
    return nlohmann::json{{}};
}}'''
    
    def generate_schema_definition(self, intent_name: str, params: Dict[str, Any]) -> str:
        """Generate JSON schema definition for an intent"""
        properties = {}
        required = []
        
        for param_name, param_info in params.items():
            properties[param_name] = param_info
            if param_info.get('required', False):
                required.append(param_name)
        
        schema_def = {
            "type": "object",
            "properties": properties,
            "required": required,
            "additionalProperties": False
        }
        
        return json.dumps(schema_def, indent=2)
    
    def intent_to_method_name(self, intent_name: str) -> str:
        """Convert intent name to camelCase method name"""
        parts = intent_name.split('_')
        return parts[0] + ''.join(word.capitalize() for word in parts[1:])
    
    def json_type_to_ts_type(self, param_info: Dict[str, Any]) -> str:
        """Convert JSON schema type to TypeScript type"""
        json_type = param_info.get('type', 'any')
        
        type_map = {
            'string': 'string',
            'number': 'number',
            'integer': 'number',
            'boolean': 'boolean',
            'array': 'any[]',
            'object': 'any'
        }
        
        return type_map.get(json_type, 'any')
    
    def json_type_to_cpp_type(self, param_info: Dict[str, Any]) -> str:
        """Convert JSON schema type to C++ type"""
        json_type = param_info.get('type', 'auto')
        
        type_map = {
            'string': 'std::string',
            'number': 'double',
            'integer': 'int',
            'boolean': 'bool',
            'array': 'nlohmann::json',
            'object': 'nlohmann::json'
        }
        
        return type_map.get(json_type, 'auto')
    
    def generate_intent(self, intent_name: str, params: Dict[str, Any], output_dir: Path):
        """Generate all code files for a new intent"""
        output_dir.mkdir(parents=True, exist_ok=True)
        
        # Generate SDK method
        sdk_code = self.generate_sdk_method(intent_name, params)
        with open(output_dir / f"{intent_name}_sdk.ts", 'w') as f:
            f.write(sdk_code)
        
        # Generate C++ handler
        cpp_code = self.generate_cpp_handler(intent_name, params)
        with open(output_dir / f"{intent_name}_handler.cpp", 'w') as f:
            f.write(cpp_code)
        
        # Generate schema definition
        schema_code = self.generate_schema_definition(intent_name, params)
        with open(output_dir / f"{intent_name}_schema.json", 'w') as f:
            f.write(schema_code)
        
        print(f"✅ Generated intent '{intent_name}' in {output_dir}")
        print("📝 Next steps:")
        print(f"  1. Add {intent_name}_sdk.ts method to sdk/web/glint3d.ts")
        print(f"  2. Add {intent_name}_handler.cpp function to engine/src/json_ops.cpp")
        print(f"  3. Add {intent_name}_schema.json to schemas/json_ops_v2.json")
        print(f"  4. Add integration tests for the new intent")

def main():
    parser = argparse.ArgumentParser(description="Generate code for new Glint3D intents")
    parser.add_argument("--intent", required=True, help="Intent name (e.g., add_material)")
    parser.add_argument("--schema", help="Path to JSON schema file")
    parser.add_argument("--output", default="generated", help="Output directory")
    
    args = parser.parse_args()
    
    # Example parameters - in practice this would come from user input or schema
    example_params = {
        "target_id": {"type": "string", "required": True},
        "material_data": {"type": "object", "required": True},
        "override_existing": {"type": "boolean", "required": False}
    }
    
    generator = IntentGenerator(Path(args.schema) if args.schema else Path("schemas/json_ops_v2.json"))
    output_dir = Path(args.output) / args.intent
    
    generator.generate_intent(args.intent, example_params, output_dir)

if __name__ == "__main__":
    main()