# cuda_stdpar

A small test project to explore CUDA's standard parallelism feature shipped with `nvc++` using WSL2.

## prerequisites

This project has been created and successfully tested using following setup:

* Windows 10 21H2 (which is the minimum required version to use CUDA in WSL as stated in the [MS documentation](https://docs.microsoft.com/en-us/windows/ai/directml/gpu-cuda-in-wsl))
* Ubuntu 22.02
* [NVIDIA HPC SDK 22.5](https://developer.nvidia.com/hpc-sdk)
* Visual Studio 17.2.4

The project possibly may also be built using previous HPC SDK versions.

After cloning this repository the `PATH` variable set in the `CMakePresets.json` file may have to be updated to include the correct path to the HPC SDK:
```{json}
"environment": {
    "PATH": "/opt/nvidia/hpc_sdk/Linux_x86_64/22.5/compilers/bin/:$penv{PATH}"
}
```

## limitations

The project can not be built with MSVC because the parallelized `std::transform` overloads require the iterators to fulfill `LegacyForwardIterator` which is not the case for the iterators of `std::views::iota`.