$OutputFile = "HashOwl_FullContext.txt"

# 清理旧文件
Clear-Content $OutputFile -ErrorAction SilentlyContinue

# 查找所有目标文件
Get-ChildItem -Path . -Include *.cpp, *.hpp, *.h, CMakeLists.txt -Recurse | 
    Where-Object { 
        $_.FullName -notmatch '\\build\\' -and 
        $_.FullName -notmatch '\\\.vs\\' -and 
        $_.FullName -notmatch '\\indicators\\' -and 
        $_.FullName -notmatch '\\json\\' -and
        $_.FullName -notmatch '\\include\\'
    } | 
    ForEach-Object { 
        # 获取相对路径
        $relativePath = $_.FullName.Substring((Get-Location).Path.Length + 1)
        
        # 写入分隔符和文件名
        Add-Content -Path $OutputFile -Value "`n// ==========================================" -Encoding UTF8
        Add-Content -Path $OutputFile -Value "// File: $relativePath" -Encoding UTF8
        Add-Content -Path $OutputFile -Value "// ==========================================`n" -Encoding UTF8
        
        # 读取内容并写入
        Get-Content $_.FullName -Encoding UTF8 | Add-Content -Path $OutputFile -Encoding UTF8
    }

Write-Host "🚀 融合完毕！已生成 $OutputFile" -ForegroundColor Cyan