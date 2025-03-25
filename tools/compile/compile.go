package main

import (
	"fmt"
	"log"
	"os"

	"slices"

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
	var config BuildsConfig

	file, err := os.ReadFile("builds.toml")
	if err != nil {
		log.Fatalf("Failed to read file: %v", err)
	}

	_, err = toml.Decode(string(file), &config)
	if err != nil {
		log.Fatalf("Failed to decode file: %v", err)
	}

	// Print all builds
	fmt.Println("OpenWrt SDK builds:")
	for i, build := range config.Builds {
		fmt.Printf("\nBuild #%d:\n", i+1)
		fmt.Printf("  Target: %s\n", build.Target)
		fmt.Printf("  Subtarget: %s\n", build.Subtarget)
		fmt.Printf("  Architecture: %s\n", build.Architecture)
		fmt.Printf("  Codenames: %v\n", build.Codenames)
	}

	// Example: Find builds for a specific codename
	targetCodename := "prometheus"
	fmt.Printf("\nBuilds supporting codename '%s':\n", targetCodename)
	for _, build := range config.Builds {
		if slices.Contains(build.Codenames, targetCodename) {
			fmt.Printf("  %s/%s (%s)\n", build.Target, build.Subtarget, build.Architecture)
		}
	}
}
