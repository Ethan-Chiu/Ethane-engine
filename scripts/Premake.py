import Config
import Utils


def check_premake_installed():
    cfg = Config.premake_config
    if not Utils.filename_exists_in_dir("premake5", cfg.install_dir):
        print("You don't have the premake installed!")
        return install_premake_prompt()

    Utils.CheckMarkMsg(f"Correct Premake.exe located at {cfg.install_dir}")
    return True


def install_premake():
    cfg = Config.premake_config
    print(f"Downloading {cfg.installer_url} to {cfg.install_dir}")
    Utils.download_archive(cfg.installer_url, cfg.install_dir)
    print("premake5.exe downloaded at: ", cfg.install_dir)
    return True


def install_premake_prompt():
    print("Would you like to install the Premake.exe?")
    install = Utils.YesOrNo()
    if install:
        return install_premake()
    else:
        return False
