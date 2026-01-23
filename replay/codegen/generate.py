import argparse
from parser import generate

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog='CodeGenerator',
        description='Generates build code')

    parser.add_argument("header_codegen_path")
    parser.add_argument("cpp_codegen_path")
    args = parser.parse_args()
    generate(args.header_codegen_path, args.cpp_codegen_path)
    print("Codegen Completed")