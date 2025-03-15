# mzQC C++ Parser

A simple C++ implementation for working with mzQC (Mass Spectrometry Quality Control) files.

## Dependencies

- C++17 or later
- ![nlohmann/json library](https://github.com/nlohmann/json)

## Running Process


You can use the built-in command-line interface to examine mzQC files:

```bash
./mzqc_parser <input_file.mzQC> [<output_file.mzQC>]
```

Example:
```bash
./mzqc_parser example.mzQC
```

This will show information about the mzQC file including its version, creation date, and metrics.

To convert or validate a file:
```bash
./mzqc_parser ex.mzQC output.mzQC
```

## Current Functions

### Data Handling

- Parse mzQC files into C++ objects
- Support for different value types (numbers, strings, arrays, tables)
- Access individual quality metrics and their values
- Extract metadata about files and controlled vocabularies

## Example Files

The repository includes example files to help you understand how the parser works:

- `example.mzQC`: A sample mzQC file that demonstrates the proper structure and format
- `output.mzQC`: The output generated when parsing and reserializing the example file


## Related Projects

- ![jmzQC](https://github.com/MS-Quality-Hub/jmzqc) - Java implementation of the mzQC standard
- ![pymzqc](https://github.com/MS-Quality-Hub/pymzqc) - Python implementation of the mzQC standard
- HUPO-PSI mzQC - Official mzQC standard repository