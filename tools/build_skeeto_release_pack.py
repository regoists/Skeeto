"""Generate the current Steam-layout development folder; no publication or archive creation.

The former historical dual-track generator is backed up with the architecture
snapshot. Official release and desktop document handling follow the release guide.
"""
from package_bootstrap_migration import main

if __name__ == "__main__":
    main()
    print("Development folder only. Formal desktop/workshop publication is a separate authorized task.")
