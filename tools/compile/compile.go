package main

import (
	"context"
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"strings"

	"github.com/BurntSushi/toml"
	"github.com/docker/docker/api/types"
	"github.com/docker/docker/api/types/container"
	"github.com/docker/docker/api/types/mount"
	"github.com/docker/docker/client"
	"github.com/docker/docker/pkg/archive"
	"github.com/docker/docker/pkg/jsonmessage"
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

	fmt.Printf("Searching for build configuration for architecture: %s, subtarget: %s\n", requestedArch, requestedSubtarget)

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
		fmt.Println("\nAvailable build configurations:")
		for _, build := range config.Builds {
			fmt.Printf("  - %s:%s (target=%s)\n", build.Architecture, build.Subtarget, build.Target)
		}
		os.Exit(1)
	}

	fmt.Printf("\nFound matching build configuration:\n")
	fmt.Printf("  Target: %s\n", selectedBuild.Target)
	fmt.Printf("  Subtarget: %s\n", selectedBuild.Subtarget)
	fmt.Printf("  Architecture: %s\n", selectedBuild.Architecture)
	fmt.Printf("  Codenames: %v\n", selectedBuild.Codenames)

	fmt.Printf("\nCompiling for architecture: %s, subtarget: %s\n", selectedBuild.Architecture, selectedBuild.Subtarget)

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

	// Clean up and set up directories
	fmt.Println("Cleaning and setting up directories")
	os.RemoveAll(feedDir)
	os.MkdirAll(filepath.Join(feedDir, "admin", "wayru-os-services"), 0755)
	archDir := fmt.Sprintf("%s_%s", selectedBuild.Architecture, selectedBuild.Subtarget)
	outputDir := filepath.Join(buildDir, archDir)
	os.MkdirAll(outputDir, 0755)

	// Copy source files into feed
	fmt.Println("Copying build source to feed directory")
	copyFile(filepath.Join(projectRoot, "Makefile"), filepath.Join(feedDir, "admin", "wayru-os-services", "Makefile"))
	copyFile(filepath.Join(projectRoot, "VERSION"), filepath.Join(feedDir, "admin", "wayru-os-services", "VERSION"))
	copyDir(filepath.Join(projectRoot, "source"), filepath.Join(feedDir, "admin", "wayru-os-services", "source"))

	// Prepare SDK paths

	var muslSuffix string
	if selectedBuild.Target == "ipq40xx" && selectedBuild.Subtarget == "mikrotik" {
		muslSuffix = "musl_eabi"
	} else {
		muslSuffix = "musl"
	}

	sdkFilename := fmt.Sprintf(
		"openwrt-sdk-%s-%s-%s_gcc-12.3.0_%s.Linux-x86_64.tar.xz",
		selectedBuild.Version,
		selectedBuild.Target,
		selectedBuild.Subtarget,
		muslSuffix,
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
			"openwrt-sdk-%s-%s-%s_gcc-12.3.0_%s.Linux-x86_64",
			selectedBuild.Version,
			selectedBuild.Target,
			selectedBuild.Subtarget,
			muslSuffix,
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

	imageName := "wayru-package-builder:latest"

	exists, err := checkDockerImageExists(imageName)
	if err != nil {
		log.Fatalf("Docker image check failed: %v", err)
	}

	if exists {
		fmt.Printf("Docker image %s already exists.\n", imageName)
	} else {
		fmt.Printf("Docker image %s not found. Building it now...\n", imageName)
		err = buildDockerImageFromDir(
			"Dockerfile.build",
			"wayru-package-builder:latest",
			projectRoot,
		)

	}

	err = runDockerContainerFromImage(
		imageName,
		sdkDir,
		outputDir,
		selectedBuild.FeedsName,
		feedDir,
	)
	if err != nil {
		log.Fatalf("Container build failed: %v", err)
	}

	fmt.Printf("\nPackage built for architecture: %s\n", selectedBuild.Architecture)
	fmt.Printf("Saved to: %s\n", outputDir)

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

func checkDockerImageExists(imageName string) (bool, error) {
	cli, err := client.NewClientWithOpts(
		client.FromEnv,
		client.WithAPIVersionNegotiation(),
	)
	if err != nil {
		return false, fmt.Errorf("Failed to create Docker client: %w", err)
	}
	defer cli.Close()

	ctx := context.Background()

	_, err = cli.ImageInspect(ctx, imageName)
	if err != nil {
		if client.IsErrNotFound(err) || strings.Contains(err.Error(), "No such image") {
			return false, nil
		}
		return false, fmt.Errorf("Error inspecting image: %w", err)
	}

	return true, nil
}

func buildDockerImageFromDir(dockerfilePath string, imageName string, contextDir string) error {
	cli, err := client.NewClientWithOpts(
		client.FromEnv,
		client.WithAPIVersionNegotiation(),
	)
	if err != nil {
		return fmt.Errorf("Failed to create Docker client: %w", err)
	}
	defer cli.Close()

	contextTar, err := archive.TarWithOptions(contextDir, &archive.TarOptions{})
	if err != nil {
		return fmt.Errorf("Failed to create tar context: %w", err)
	}

	buildOptions := types.ImageBuildOptions{
		Tags:       []string{imageName},
		Dockerfile: dockerfilePath,
		Remove:     true,
	}

	response, err := cli.ImageBuild(context.Background(), contextTar, buildOptions)
	if err != nil {
		return fmt.Errorf("Failed to build Docker image: %w", err)
	}
	defer response.Body.Close()

	err = jsonmessage.DisplayJSONMessagesStream(response.Body, os.Stdout, os.Stdout.Fd(), true, nil)
	if err != nil {
		return fmt.Errorf("Error reading build output: %w", err)
	}

	return nil
}

func runDockerContainerFromImage(
	imageName string,
	sdkHostPath string,
	outputHostPath string,
	feedsName string,
	feedDir string,
) error {
	ctx := context.Background()

	cli, err := client.NewClientWithOpts(client.FromEnv, client.WithAPIVersionNegotiation())
	if err != nil {
		return fmt.Errorf("Error creating Docker client: %w", err)
	}
	defer cli.Close()

	containerConfig := &container.Config{
		Image: imageName,
		Cmd: []string{"/bin/bash", "-c", fmt.Sprintf(`
			set -e
			cd /sdk
			echo "src-link %[1]s /feed" > feeds.conf
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

			echo "Moving compiled package to /output"
			mkdir -p /output
			find bin/packages -type f -name 'wayru-os-services_*.ipk' -exec cp {} /output/ \;

			make package/wayru-os-services/clean
		`, feedsName)},
		Tty: true,
	}

	hostConfig := &container.HostConfig{
		Mounts: []mount.Mount{
			{
				Type:   mount.TypeBind,
				Source: sdkHostPath,
				Target: "/sdk",
			},
			{
				Type:   mount.TypeBind,
				Source: outputHostPath,
				Target: "/output",
			},
			{
				Type:   mount.TypeBind,
				Source: feedDir,
				Target: "/feed",
			},
		},
	}

	resp, err := cli.ContainerCreate(ctx, containerConfig, hostConfig, nil, nil, "")
	if err != nil {
		return fmt.Errorf("Error creating container: %w", err)
	}

	if err := cli.ContainerStart(ctx, resp.ID, container.StartOptions{}); err != nil {
		return fmt.Errorf("Error starting container: %w", err)
	}

	logReader, err := cli.ContainerLogs(ctx, resp.ID, container.LogsOptions{
		ShowStdout: true,
		ShowStderr: true,
		Follow:     true,
		Timestamps: false,
	})
	if err != nil {
		return fmt.Errorf("Error getting container logs: %w", err)
	}
	defer logReader.Close()

	go func() {
		_, _ = io.Copy(os.Stdout, logReader)
	}()

	statusCh, errCh := cli.ContainerWait(ctx, resp.ID, container.WaitConditionNotRunning)
	select {
	case err := <-errCh:
		if err != nil {
			return fmt.Errorf("Error while waiting for container: %w", err)
		}
	case status := <-statusCh:
		if status.StatusCode != 0 {
			return fmt.Errorf("Container exited with status code: %d", status.StatusCode)
		}
	}

	// Delete container
	err = cli.ContainerRemove(ctx, resp.ID, container.RemoveOptions{
		RemoveVolumes: true,
		Force:         true,
	})
	if err != nil {
		fmt.Printf("Warning: failed to remove container: %v\n", err)
	}

	return nil
}
