package main

import (
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"time"

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
	Version      string   `toml:"version"`
	FeedsName    string   `toml:"feedsName"`
}

func main() {
	if len(os.Args) < 2 {
		log.Fatal("Please provide an architecture as an argument")
	}

	archArg := os.Args[1]
	requestedArch := archArg
	requestedSubtarget := ""

	if parts := strings.Split(archArg, ":"); len(parts) == 2 {
		requestedArch = parts[0]
		requestedSubtarget = parts[1]
	}

	fmt.Printf("Searching for build configuration for architecture: %s\n", requestedArch)

	var config BuildsConfig
	file, err := os.ReadFile("builds.toml")
	if err != nil {
		log.Fatalf("Failed to read file: %v", err)
	}

	_, err = toml.Decode(string(file), &config)
	if err != nil {
		log.Fatalf("Failed to decode file: %v", err)
	}

	var selectedBuild *Build
	for _, build := range config.Builds {
		if build.Architecture == requestedArch {
			if requestedSubtarget == "" || build.Subtarget == requestedSubtarget {
				selectedBuild = &build
				break
			}
		}
	}

	if selectedBuild == nil {
		fmt.Printf("\nNo build configuration found for architecture: %s\n", requestedArch)
		fmt.Println("\nAvailable architectures:")
		for _, build := range config.Builds {
			fmt.Printf("  - %s\n", build.Architecture)
		}
		os.Exit(1)
	}

	fmt.Printf("\nFound matching build configuration:\n")
	fmt.Printf("  Target: %s\n", selectedBuild.Target)
	fmt.Printf("  Subtarget: %s\n", selectedBuild.Subtarget)
	fmt.Printf("  Architecture: %s\n", selectedBuild.Architecture)
	fmt.Printf("  Codenames: %v\n", selectedBuild.Codenames)

	fmt.Printf("\nCompiling for architecture: %s\n", selectedBuild.Architecture)

	currentDir, _ := os.Getwd()
	projectRoot := filepath.Join(currentDir, "..", "..")

	// Directories
	buildDir := filepath.Join(projectRoot, "build")
	feedDir := filepath.Join(projectRoot, "feed")
	//sdkDir := filepath.Join(projectRoot, fmt.Sprintf("sdk_%s", selectedBuild.Architecture))
	sdkDir := filepath.Join(
		projectRoot,
		fmt.Sprintf("sdk_%s_%s", selectedBuild.Architecture, selectedBuild.Subtarget),
	)

	timestamp := time.Now().Format("2006-01-02-150405")
	versionedBuildDir := filepath.Join(buildDir, timestamp)

	// Clean up and set up directories
	fmt.Println("Cleaning and setting up directories")
	os.RemoveAll(feedDir)
	os.MkdirAll(filepath.Join(feedDir, "admin", "wayru-os-services"), 0755)
	os.MkdirAll(versionedBuildDir, 0755)

	// Copy source files into feed
	fmt.Println("Copying build source to feed directory")
	copyFile(filepath.Join(projectRoot, "Makefile"), filepath.Join(feedDir, "admin", "wayru-os-services", "Makefile"))
	copyFile(filepath.Join(projectRoot, "VERSION"), filepath.Join(feedDir, "admin", "wayru-os-services", "VERSION"))
	copyDir(filepath.Join(projectRoot, "source"), filepath.Join(feedDir, "admin", "wayru-os-services", "source"))

	// Prepare SDK paths
	sdkFilename := fmt.Sprintf(
		"openwrt-sdk-%s-%s-%s_gcc-12.3.0_musl.Linux-x86_64.tar.xz",
		selectedBuild.Version,
		selectedBuild.Target,
		selectedBuild.Subtarget,
	)
	sdkURL := fmt.Sprintf(
		"https://archive.openwrt.org/releases/%s/targets/%s/%s/%s",
		selectedBuild.Version,
		selectedBuild.Target,
		selectedBuild.Subtarget,
		sdkFilename,
	)

	if _, err := os.Stat(sdkDir); os.IsNotExist(err) {
		fmt.Println("SDK directory does not exist; proceeding to download and extract.")

		// Download tar.xz
		sdkArchivePath := filepath.Join(projectRoot, sdkFilename)
		if _, err := os.Stat(sdkArchivePath); os.IsNotExist(err) {
			fmt.Printf("Downloading SDK from: %s\n", sdkURL)
			err := downloadFile(sdkURL, sdkArchivePath)
			if err != nil {
				log.Fatalf("Failed to download SDK: %v", err)
			}
			fmt.Println("SDK download completed.")
		} else {
			fmt.Println("SDK archive already downloaded.")
		}

		// Extract SDK
		fmt.Printf("Extracting SDK to: %s\n", sdkDir)
		err := extractSDK(sdkArchivePath, projectRoot)
		if err != nil {
			log.Fatalf("Failed to extract SDK: %v", err)
		}

		// Rename extracted SDK directory
		sdkExtractedName := fmt.Sprintf(
			"openwrt-sdk-%s-%s-%s_gcc-12.3.0_musl.Linux-x86_64",
			selectedBuild.Version,
			selectedBuild.Target,
			selectedBuild.Subtarget,
		)
		err = os.Rename(filepath.Join(projectRoot, sdkExtractedName), sdkDir)
		if err != nil {
			log.Fatalf("Failed to rename SDK directory: %v", err)
		}
		fmt.Println("SDK extraction completed.")

		// Remove tar.xz
		err = os.Remove(sdkArchivePath)
		if err != nil {
			fmt.Printf("Warning: failed to remove SDK archive %s: %v\n", sdkArchivePath, err)
		} else {
			fmt.Printf("Removed SDK archive: %s\n", sdkArchivePath)
		}

	} else {
		fmt.Println("SDK already extracted and ready to use.")
	}

	// Run Docker to compile
	fmt.Println("Running Docker build...")

	err = runDocker(
		sdkDir,
		feedDir,
		selectedBuild.FeedsName,
		versionedBuildDir,
	)
	if err != nil {
		log.Fatalf("Docker run failed: %v", err)
	}

	fmt.Printf("\n Build completed for architecture: %s\n", selectedBuild.Architecture)
	fmt.Printf("Package saved to: %s\n", versionedBuildDir)
}

func downloadFile(url string, filepath string) error {
	resp, err := http.Get(url)
	if err != nil {
		return err
	}
	defer resp.Body.Close()

	if resp.StatusCode != 200 {
		return fmt.Errorf("Failed to download file: HTTP %d", resp.StatusCode)
	}

	out, err := os.Create(filepath)
	if err != nil {
		return err
	}
	defer out.Close()

	_, err = io.Copy(out, resp.Body)
	return err
}

func extractSDK(archive string, extractTo string) error {
	cmd := exec.Command("tar", "-xf", archive, "-C", extractTo)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	return cmd.Run()
}

func runDocker(sdkPath string, feedsPath string, feedsName string, outputDir string) error {
	cmd := exec.Command(
		"docker", "run", "--rm",
		"-v", fmt.Sprintf("%s:/sdk", sdkPath),
		"-v", fmt.Sprintf("%s:/feeds", feedsPath),
		"-v", fmt.Sprintf("%s:/output", outputDir),
		"-w", "/sdk",
		"openwrt-builder",
		"/bin/bash", "-c",
		fmt.Sprintf(`
            echo "src-link %s /feeds" > feeds.conf
            cat feeds.conf.default >> feeds.conf
            sed -i '/luci/d' feeds.conf
            sed -i '/telephony/d' feeds.conf
            sed -i '/routing/d' feeds.conf
            ./scripts/feeds update -a
            ./scripts/feeds install wayru-os-services
            rm -f .config
            touch .config
            echo "# CONFIG_ALL_NONSHARED is not set" >> .config
            echo "# CONFIG_ALL_KMODS is not set" >> .config
            echo "# CONFIG_ALL is not set" >> .config
            echo "CONFIG_PACKAGE_wayru-os-services=y" >> .config
            make defconfig
            make package/wayru-os-services/download
            make package/wayru-os-services/prepare
            make package/wayru-os-services/compile
            ARCH_DIR=$(grep 'CONFIG_TARGET_ARCH_PACKAGES=' .config | sed -E 's/CONFIG_TARGET_ARCH_PACKAGES=\"([^\"]+)\"/\\1/')
            mkdir -p /output/$ARCH_DIR
            cp -r bin/packages/$ARCH_DIR/%s/* /output/$ARCH_DIR/
            make package/wayru-os-services/clean
        `, feedsName, feedsName),
	)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	return cmd.Run()
}

func copyFile(src, dst string) {
	input, err := os.ReadFile(src)
	if err != nil {
		log.Fatalf("Failed to read file %s: %v", src, err)
	}
	err = os.WriteFile(dst, input, 0644)
	if err != nil {
		log.Fatalf("Failed to write file %s: %v", dst, err)
	}
}

func copyDir(src string, dst string) {
	cmd := exec.Command("cp", "-r", src, dst)
	err := cmd.Run()
	if err != nil {
		log.Fatalf("Failed to copy directory %s to %s: %v", src, dst, err)
	}
}
