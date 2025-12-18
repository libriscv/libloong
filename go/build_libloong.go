//go:build ignore
// +build ignore

// This program builds libloong C++ library and wrapper
// It's invoked by go generate
package main

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
)

func main() {
	// Get the current working directory (where go generate was run)
	scriptDir, err := os.Getwd()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to get working directory: %v\n", err)
		os.Exit(1)
	}

	// Verify we're in the go directory by checking for go.mod
	if _, err := os.Stat(filepath.Join(scriptDir, "go.mod")); os.IsNotExist(err) {
		// Try to find the go directory
		if filepath.Base(scriptDir) != "go" {
			scriptDir = filepath.Join(scriptDir, "go")
		}
	}

	rootDir := filepath.Join(scriptDir, "..")
	buildDir := filepath.Join(rootDir, "build")
	libDir := filepath.Join(buildDir, "lib")
	wrapperDir := filepath.Join(rootDir, "rust", "wrapper")
	cppLibDir := filepath.Join(rootDir, "lib")
	goDir := scriptDir

	// Check if libloong already exists
	libPath := filepath.Join(libDir, "libloong.a")
	if _, err := os.Stat(libPath); err == nil {
		fmt.Println("libloong.a already built")
	} else {
		fmt.Println("Building libloong C++ library...")

		// Create build directory
		if err := os.MkdirAll(buildDir, 0755); err != nil {
			fmt.Fprintf(os.Stderr, "Failed to create build directory: %v\n", err)
			os.Exit(1)
		}

		// Determine CMake generator (prefer Ninja)
		generator := "Unix Makefiles"
		if _, err := exec.LookPath("ninja"); err == nil {
			generator = "Ninja"
		}

		// Configure with CMake
		cmakeArgs := []string{
			"-G", generator,
			"-DCMAKE_BUILD_TYPE=Release",
			"-S", rootDir,
			"-B", buildDir,
		}

		cmd := exec.Command("cmake", cmakeArgs...)
		cmd.Dir = buildDir
		cmd.Stdout = os.Stdout
		cmd.Stderr = os.Stderr
		if err := cmd.Run(); err != nil {
			fmt.Fprintf(os.Stderr, "CMake configure failed: %v\n", err)
			os.Exit(1)
		}

		// Build the loong target
		var buildCmd *exec.Cmd
		if generator == "Ninja" {
			buildCmd = exec.Command("ninja", "loong")
		} else {
			buildCmd = exec.Command("make", "loong", "-j"+fmt.Sprint(runtime.NumCPU()))
		}
		buildCmd.Dir = buildDir
		buildCmd.Stdout = os.Stdout
		buildCmd.Stderr = os.Stderr
		if err := buildCmd.Run(); err != nil {
			fmt.Fprintf(os.Stderr, "Build failed: %v\n", err)
			os.Exit(1)
		}

		fmt.Println("✓ libloong built successfully")
	}

	// Build C wrapper
	fmt.Println("Building C wrapper...")

	wrapperObj := filepath.Join(goDir, "libloong_wrapper.o")
	wrapperLib := filepath.Join(goDir, "libloong_wrapper.a")

	// Determine C++ compiler
	cxx := os.Getenv("CXX")
	if cxx == "" {
		cxx = "g++"
		// Try to find a newer version
		for _, candidate := range []string{"g++-14", "g++-13", "clang++-18", "clang++-17", "clang++"} {
			if _, err := exec.LookPath(candidate); err == nil {
				cxx = candidate
				break
			}
		}
	}

	// Compile wrapper
	compileArgs := []string{
		"-std=c++20",
		"-c",
		"-O3",
		"-fPIC",
		"-I" + wrapperDir,
		"-I" + cppLibDir,
		"-I" + libDir,
		filepath.Join(wrapperDir, "libloong_wrapper.cpp"),
		"-o", wrapperObj,
	}

	cmd := exec.Command(cxx, compileArgs...)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	if err := cmd.Run(); err != nil {
		fmt.Fprintf(os.Stderr, "Wrapper compilation failed: %v\n", err)
		os.Exit(1)
	}

	// Create static library
	arCmd := exec.Command("ar", "rcs", wrapperLib, wrapperObj)
	arCmd.Stdout = os.Stdout
	arCmd.Stderr = os.Stderr
	if err := arCmd.Run(); err != nil {
		fmt.Fprintf(os.Stderr, "Archive creation failed: %v\n", err)
		os.Exit(1)
	}

	fmt.Println("✓ C wrapper built successfully")
	fmt.Println("\nBuild complete!")
}
