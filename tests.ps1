# Simple test runner: builds and runs several traces, saving their outputs
$ErrorActionPreference = 'Stop'

# Ensure bin exists
New-Item -ItemType Directory -Path bin -Force | Out-Null

# Compile
Write-Host "Compiling..."
g++ -g -O0 -I . -o .\bin\interrupts.exe .\interrupts.cpp

# Test cases (array of [traceFile, label])
$tests = @(
    @{file='trace.txt'; label='original'},
    @{file='trace_exec.txt'; label='exec'},
    @{file='trace_fork.txt'; label='fork'}
)

foreach ($t in $tests) {
    $trace = $t.file
    $label = $t.label
    Write-Host "Running test: $label (trace: $trace)"

    .\bin\interrupts.exe .\$trace .\vector_table.txt .\device_table.txt .\external_files.txt

    # Copy results to distinct files to preserve each run
    Copy-Item .\execution.txt .\execution_$label.txt -Force
    Copy-Item .\system_status.txt .\system_status_$label.txt -Force

    Write-Host "Saved execution_$label.txt and system_status_$label.txt"
}

Write-Host "All tests done."
