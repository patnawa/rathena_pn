param(
    [string]$OutputPath = (Join-Path $PSScriptRoot "..\db\import\zero_cell_mob_db.yml")
)

$ErrorActionPreference = "Stop"

# id,aegis,name,level,hp,def,mdef,res,mres,race,element,element-level,damage-taken,attack
$rows = @'
22669,G_CH1_RK_ER,M.RK-ERROR,272,129374115,539,812,926,1236,Demihuman,Fire,2,2,34000
22670,G_CH1_GC_ER,M.GC-ERROR,272,118473799,527,794,1094,870,Demon,Poison,4,2,35000
22671,G_CH1_GC_D_ER,M.GC.D-ERROR,272,118476711,540,815,967,976,Demon,Dark,4,2,36000
22507,CH1_RK,M.RK-4480,272,129374115,539,812,926,1236,Demihuman,Fire,2,2,34000
22508,CH1_MC_M,M.MC.M-5407,271,145889584,522,786,905,833,Demihuman,Water,3,2,33000
22509,CH1_SC,M.SC-3941,270,113920556,518,780,899,811,Demon,Poison,4,2,35000
22510,CH1_RA,M.RA-4152,271,121732891,318,746,1175,1076,Demihuman,Wind,4,2,32000
22511,CH1_SO,M.SO-4206,272,111349046,384,1357,908,1288,Formless,Ghost,2,2,36000
22512,CH1_AB,M.AB-4084,272,129371365,526,792,1224,1235,Demihuman,Holy,3,2,33000
22513,CH1_MI_D,M.MI.D-5794,270,136834190,517,779,1134,1149,Demihuman,Earth,4,2,35000
22514,CH1_GC_D,M.GC.D-4402,272,118476711,540,815,967,976,Demon,Dark,4,2,36000
22516,CH1_RG,M.RG-5520,272,145696328,648,1003,992,930,Demihuman,Holy,2,2,36000
22517,CH1_MC,M.MC-4593,272,129371365,526,792,877,1082,Demihuman,Dark,3,2,35000
22518,CH1_GN,M.GN-6059,271,126869897,534,805,1193,957,Demihuman,Fire,4,2,34000
22519,CH1_GC,M.GC-5848,272,118473799,527,794,1094,870,Demon,Poison,4,2,36000
22520,CH1_MI,M.MI-4419,270,119364856,316,739,1180,993,Demihuman,Wind,2,2,33000
22521,CH1_SR,M.SR-5916,271,145889584,522,786,1038,921,Demihuman,Water,3,2,36000
22522,CH1_WL,M.WL-5581,272,111349046,384,1357,989,1313,Formless,Ghost,3,2,37000
22523,CH1_WA,M.WA-5598,270,124407240,555,837,1104,1114,Demihuman,Wind,3,2,34000
22525,CH1_RK_1,M.RK-0001,283,516663215,760,1177,1016,1091,Demihuman,Fire,3,1,47000
22526,CH1_MC_M_1,M.MC.M-0001,282,583204061,732,1134,1141,1258,Demihuman,Water,4,1,46000
22527,CH1_SC_1,M.SC-0001,281,455879766,727,1126,1210,950,Demon,Poison,4,1,48000
22528,CH1_RA_1,M.RA-0001,282,486634384,437,1091,1278,983,Demihuman,Wind,4,1,45000
22529,CH1_SO_1,M.SO-0001,283,444691488,548,2038,1180,1420,Formless,Ghost,3,1,50000
22530,CH1_AB_1,M.AB-0001,283,516650486,739,1144,1164,923,Demihuman,Holy,4,1,46000
22531,CH1_MI_D_1,M.MI.D-0001,281,547567506,726,1124,1295,1271,Demihuman,Earth,4,1,48000
22532,CH1_GC_D_1,M.GC.D-0001,283,473146930,762,1181,1220,1110,Demon,Dark,4,1,50000
22542,CH1_RG_1,M.RG-0001,283,581844108,917,1461,1210,1120,Demihuman,Holy,3,1,50000
22543,CH1_MC_1,M.MC-0001,283,516650486,739,1144,1150,1240,Demihuman,Dark,4,1,49000
22544,CH1_GN_1,M.GN-0001,282,507177447,753,1166,1320,1040,Demihuman,Fire,4,1,48000
22545,CH1_GC_1,M.GC-0001,283,473133453,740,1146,1240,990,Demon,Poison,4,1,50000
22546,CH1_MI_1,M.MI-0001,281,477657968,433,1081,1290,1080,Demihuman,Wind,3,1,46000
22547,CH1_SR_1,M.SR-0001,282,583204061,732,1134,1170,1020,Demihuman,Water,4,1,50000
22548,CH1_WL_1,M.WL-0001,283,444691488,548,2038,1120,1450,Formless,Ghost,4,1,52000
22549,CH1_WA_1,M.WA-0001,281,497846677,787,1219,1230,1190,Demihuman,Wind,4,1,47000
'@ -split "`r?`n" | Where-Object { $_ }

$builder = [System.Text.StringBuilder]::new()
[void]$builder.AppendLine("Header:")
[void]$builder.AppendLine("  Type: MOB_DB")
[void]$builder.AppendLine("  Version: 5")
[void]$builder.AppendLine("")
[void]$builder.AppendLine("Body:")

foreach ($row in $rows) {
    $v = $row.Split(',')
    $level = [int]$v[3]
    $attack = [int]$v[13]
    $stat = if ($level -ge 280) { 310 } else { 285 }
    $dex = if ($level -ge 280) { 330 } else { 300 }
    $walk = if ($v[9] -eq "Formless") { 120 } else { 140 }

    [void]$builder.AppendLine("  - Id: $($v[0])")
    [void]$builder.AppendLine("    AegisName: $($v[1])")
    [void]$builder.AppendLine("    Name: $($v[2])")
    [void]$builder.AppendLine("    Level: $level")
    [void]$builder.AppendLine("    Hp: $($v[4])")
    [void]$builder.AppendLine("    BaseExp: 1")
    [void]$builder.AppendLine("    JobExp: 1")
    [void]$builder.AppendLine("    Attack: $attack")
    [void]$builder.AppendLine("    Attack2: $($attack + 5000)")
    [void]$builder.AppendLine("    Defense: $($v[5])")
    [void]$builder.AppendLine("    MagicDefense: $($v[6])")
    [void]$builder.AppendLine("    Resistance: $($v[7])")
    [void]$builder.AppendLine("    MagicResistance: $($v[8])")
    [void]$builder.AppendLine("    Str: $stat")
    [void]$builder.AppendLine("    Agi: $stat")
    [void]$builder.AppendLine("    Vit: $stat")
    [void]$builder.AppendLine("    Int: $stat")
    [void]$builder.AppendLine("    Dex: $dex")
    [void]$builder.AppendLine("    Luk: 180")
    [void]$builder.AppendLine("    AttackRange: 1")
    [void]$builder.AppendLine("    SkillRange: 10")
    [void]$builder.AppendLine("    ChaseRange: 12")
    [void]$builder.AppendLine("    Size: Medium")
    [void]$builder.AppendLine("    Race: $($v[9])")
    [void]$builder.AppendLine("    Element: $($v[10])")
    [void]$builder.AppendLine("    ElementLevel: $($v[11])")
    [void]$builder.AppendLine("    WalkSpeed: $walk")
    [void]$builder.AppendLine("    AttackDelay: 480")
    [void]$builder.AppendLine("    AttackMotion: 480")
    [void]$builder.AppendLine("    ClientAttackMotion: 360")
    [void]$builder.AppendLine("    DamageMotion: 360")
    [void]$builder.AppendLine("    DamageTaken: $($v[12])")
    [void]$builder.AppendLine("    Ai: '21'")
    [void]$builder.AppendLine("    Class: Boss")
}

$resolved = [System.IO.Path]::GetFullPath($OutputPath)
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
[System.IO.File]::WriteAllText($resolved, $builder.ToString(), $utf8NoBom)
Write-Output "Generated $resolved with $($rows.Count) monsters."
