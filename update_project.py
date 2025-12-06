#!/usr/bin/env python3
"""
自動更新 Visual Studio 專案文件
"""

import os
import sys
import xml.etree.ElementTree as ET

def update_vcxproj():
    """更新 .vcxproj 文件以包含新的源文件"""
    
    vcxproj_file = "TexasHoldem.vcxproj"
    
    if not os.path.exists(vcxproj_file):
        print(f"Error: {vcxproj_file} not found!")
        return False
    
    print(f"Updating {vcxproj_file}...")
    
    # 定義命名空間
    ns = {'': 'http://schemas.microsoft.com/developer/msbuild/2003'}
    ET.register_namespace('', ns[''])
    
    try:
        # 讀取專案文件
        tree = ET.parse(vcxproj_file)
        root = tree.getroot()
        
        # 找到 ClCompile 和 ClInclude 的 ItemGroup
        cpp_files = [
            r"server\Card.cpp",
            r"server\Game.cpp",
            r"server\HandEvaluator.cpp",
            r"server\main.cpp",
            r"server\Player.cpp",
            r"server\Room.cpp",
            r"server\Server.cpp",
            r"server\Session.cpp"
        ]
        
        h_files = [
            r"server\Card.h",
            r"server\Game.h",
            r"server\HandEvaluator.h",
            r"server\Player.h",
            r"server\Room.h",
            r"server\Server.h",
            r"server\Session.h"
        ]
        
        # 清除舊的 ItemGroup
        for item_group in root.findall('.//{http://schemas.microsoft.com/developer/msbuild/2003}ItemGroup'):
            # 如果包含 ClCompile 或 ClInclude，則清除
            if item_group.find('{http://schemas.microsoft.com/developer/msbuild/2003}ClCompile') is not None:
                root.remove(item_group)
            elif item_group.find('{http://schemas.microsoft.com/developer/msbuild/2003}ClInclude') is not None:
                root.remove(item_group)
        
        # 創建新的 ClCompile ItemGroup
        cpp_group = ET.SubElement(root, '{http://schemas.microsoft.com/developer/msbuild/2003}ItemGroup')
        for cpp_file in cpp_files:
            cpp_elem = ET.SubElement(cpp_group, '{http://schemas.microsoft.com/developer/msbuild/2003}ClCompile')
            cpp_elem.set('Include', cpp_file)
        
        # 創建新的 ClInclude ItemGroup
        h_group = ET.SubElement(root, '{http://schemas.microsoft.com/developer/msbuild/2003}ItemGroup')
        for h_file in h_files:
            h_elem = ET.SubElement(h_group, '{http://schemas.microsoft.com/developer/msbuild/2003}ClInclude')
            h_elem.set('Include', h_file)
        
        # 備份原文件
        backup_file = vcxproj_file + ".backup"
        if os.path.exists(vcxproj_file):
            with open(vcxproj_file, 'r', encoding='utf-8') as f:
                content = f.read()
            with open(backup_file, 'w', encoding='utf-8') as f:
                f.write(content)
            print(f"Backup created: {backup_file}")
        
        # 寫入更新後的文件
        tree.write(vcxproj_file, encoding='utf-8', xml_declaration=True)
        print(f"✓ {vcxproj_file} updated successfully!")
        print(f"\nAdded files:")
        print("  Source files:")
        for f in cpp_files:
            print(f"    - {f}")
        print("  Header files:")
        for f in h_files:
            print(f"    - {f}")
        
        return True
        
    except Exception as e:
        print(f"Error updating {vcxproj_file}: {e}")
        return False

def main():
    print("=== Visual Studio Project File Updater ===\n")
    
    # 檢查是否在正確的目錄
    if not os.path.exists("TexasHoldem.vcxproj"):
        print("Error: Please run this script from the project root directory")
        print("(The directory containing TexasHoldem.vcxproj)")
        sys.exit(1)
    
    # 警告
    print("⚠ Warning: This will modify TexasHoldem.vcxproj")
    print("A backup will be created as TexasHoldem.vcxproj.backup\n")
    
    response = input("Continue? (y/n): ").strip().lower()
    if response != 'y':
        print("Cancelled.")
        sys.exit(0)
    
    print()
    if update_vcxproj():
        print("\n✓ Update complete!")
        print("\nNext steps:")
        print("1. Open TexasHoldem.sln in Visual Studio")
        print("2. Configure Boost library paths (see VISUAL_STUDIO_SETUP.md)")
        print("3. Build the solution (F7)")
    else:
        print("\n✗ Update failed!")
        sys.exit(1)

if __name__ == "__main__":
    main()
