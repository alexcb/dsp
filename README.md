## depends
pulseaudio devel libs

https://github.com/mborgerding/kissfft comes bundled in git for easier building.

## latenct settings

see http://juho.tykkala.fi/Pulseaudio-and-latency for instructions.

Here's what I ran:

    pactl unload-module 7

	# low-latency
	pactl load-module module-alsa-card device_id="0" name="pci-0000_00_1f.3" card_name="alsa_card.pci-0000_00_1f.3" namereg_fail=false tsched=no fixed_latency_range=yes ignore_dB=no deferred_volume=yes use_ucm=yes card_properties="module-udev-detect.discovered=1" fragments=1 fragment_size=15

    # to restore original:
    pactl load-module module-alsa-card device_id="0" name="pci-0000_00_1f.3" card_name="alsa_card.pci-0000_00_1f.3" namereg_fail=false tsched=yes fixed_latency_range=no ignore_dB=no deferred_volume=yes use_ucm=yes card_properties="module-udev-detect.discovered=1"

