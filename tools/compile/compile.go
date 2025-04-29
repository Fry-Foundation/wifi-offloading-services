package main

import (
	"fmt"
	"log"
	"os"

	"github.com/BurntSushi/toml"
)

type BuildsConfig struct {
	Builds []Build `toml:"builds"`
}

type Build struct {
	Target       string   `toml:"target"`
	Subtarget    string   `toml:"subtarget"`
	Architecture string   `toml:"architecture"`
	Codenames    []string `toml:"codenames"`
}

func main() {
	if len(os.Args) < 2 {
		log.Fatal("Please provide an architecture as an argument")
	}
	arch := os.Args[1]
	fmt.Printf("Searching for build configuration for architecture: %s\n", arch)

	var config BuildsConfig

	file, err := os.ReadFile("builds.toml")
	if err != nil {
		log.Fatalf("Failed to read file: %v", err)
	}

	_, err = toml.Decode(string(file), &config)
	if err != nil {
		log.Fatalf("Failed to decode file: %v", err)
	}

	// Find and print the matching build configuration
	found := false
	for _, build := range config.Builds {
		if build.Architecture == arch {
			found = true
			fmt.Printf("\nFound matching build configuration:\n")
			fmt.Printf("  Target: %s\n", build.Target)
			fmt.Printf("  Subtarget: %s\n", build.Subtarget)
			fmt.Printf("  Architecture: %s\n", build.Architecture)
			fmt.Printf("  Codenames: %v\n", build.Codenames)
			break
		}
	}

	if !found {
		fmt.Printf("\nNo build configuration found for architecture: %s\n", arch)
		fmt.Println("\nAvailable architectures:")
		for _, build := range config.Builds {
			fmt.Printf("  - %s\n", build.Architecture)
		}
	}
}
