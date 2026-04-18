#!/usr/bin/env python3
"""
CERN ROOT を使った PID チューニングデータの可視化

使い方:
  python3 tools/visualize_root.py tools/data/pid_XXXXXXXX_XXXXXX.csv

出力:
  - Angle vs time グラフ
  - Power vs time グラフ
  - Angle vs Power 散布図
  - .root ファイルに保存
"""

import sys
import os

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 tools/visualize_root.py <csv_file>")
        print("  csv_file: tools/data/pid_XXXXXXXX_XXXXXX.csv")
        return
    
    csv_file = sys.argv[1]
    
    try:
        import ROOT
    except ImportError:
        print("CERN ROOT is not installed.")
        print("Install with: brew install root")
        print("Or: conda install -c conda-forge root")
        print("\nFalling back to matplotlib...")
        visualize_matplotlib(csv_file)
        return
    
    visualize_root(csv_file)

def visualize_root(csv_file):
    import ROOT
    
    # データ読み込み
    timestamps, angles, powers, powerLs, powerRs = load_csv(csv_file)
    
    if not timestamps:
        print("No data found")
        return
    
    n = len(timestamps)
    t0 = timestamps[0]
    times = [t - t0 for t in timestamps]
    
    # ROOT ファイル作成
    root_file = csv_file.replace(".csv", ".root")
    f = ROOT.TFile(root_file, "RECREATE")
    
    # TGraph: Angle vs time
    g_angle = ROOT.TGraph(n)
    g_angle.SetName("g_angle")
    g_angle.SetTitle("Angle vs Time;Time [s];Angle [deg]")
    for i in range(n):
        g_angle.SetPoint(i, times[i], angles[i])
    g_angle.SetLineColor(ROOT.kBlue)
    g_angle.SetMarkerStyle(1)
    
    # TGraph: Power vs time
    g_power = ROOT.TGraph(n)
    g_power.SetName("g_power")
    g_power.SetTitle("Power vs Time;Time [s];Power [us]")
    for i in range(n):
        g_power.SetPoint(i, times[i], powers[i])
    g_power.SetLineColor(ROOT.kRed)
    g_power.SetMarkerStyle(1)
    
    # TGraph: Angle vs Power
    g_ap = ROOT.TGraph(n)
    g_ap.SetName("g_angle_power")
    g_ap.SetTitle("Angle vs Power;Angle [deg];Power [us]")
    for i in range(n):
        g_ap.SetPoint(i, angles[i], powers[i])
    g_ap.SetMarkerStyle(6)
    g_ap.SetMarkerColor(ROOT.kGreen + 2)
    
    # TH1D: Angle distribution
    h_angle = ROOT.TH1D("h_angle", "Angle Distribution;Angle [deg];Count", 100, -45, 45)
    for a in angles:
        h_angle.Fill(a)
    
    # Canvas
    c = ROOT.TCanvas("c_pid", "PID Tuning Analysis", 1200, 800)
    c.Divide(2, 2)
    
    c.cd(1)
    g_angle.Draw("AL")
    
    c.cd(2)
    g_power.Draw("AL")
    
    c.cd(3)
    g_ap.Draw("AP")
    
    c.cd(4)
    h_angle.Draw()
    
    # 保存
    c.Write()
    g_angle.Write()
    g_power.Write()
    g_ap.Write()
    h_angle.Write()
    f.Close()
    
    # PNG出力
    png_file = csv_file.replace(".csv", ".png")
    c.SaveAs(png_file)
    
    print(f"ROOT file: {root_file}")
    print(f"PNG file: {png_file}")
    print(f"Data points: {n}")
    print(f"Angle range: [{min(angles):.1f}, {max(angles):.1f}]")
    print(f"Power range: [{min(powers):.0f}, {max(powers):.0f}]")

def visualize_matplotlib(csv_file):
    """ROOT が無い場合のフォールバック"""
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    
    timestamps, angles, powers, powerLs, powerRs = load_csv(csv_file)
    
    if not timestamps:
        print("No data found")
        return
    
    t0 = timestamps[0]
    times = [t - t0 for t in timestamps]
    
    fig, axes = plt.subplots(2, 2, figsize=(12, 8))
    fig.suptitle(f"PID Tuning Data ({os.path.basename(csv_file)})")
    
    axes[0, 0].plot(times, angles, 'b-', linewidth=0.5)
    axes[0, 0].set_xlabel("Time [s]")
    axes[0, 0].set_ylabel("Angle [deg]")
    axes[0, 0].set_title("Angle vs Time")
    axes[0, 0].axhline(y=0, color='gray', linestyle='--')
    
    axes[0, 1].plot(times, powers, 'r-', linewidth=0.5)
    axes[0, 1].set_xlabel("Time [s]")
    axes[0, 1].set_ylabel("Power [μs]")
    axes[0, 1].set_title("Power vs Time")
    
    axes[1, 0].scatter(angles, powers, s=1, c='green', alpha=0.5)
    axes[1, 0].set_xlabel("Angle [deg]")
    axes[1, 0].set_ylabel("Power [μs]")
    axes[1, 0].set_title("Angle vs Power")
    
    axes[1, 1].hist(angles, bins=50, color='blue', alpha=0.7)
    axes[1, 1].set_xlabel("Angle [deg]")
    axes[1, 1].set_ylabel("Count")
    axes[1, 1].set_title("Angle Distribution")
    
    plt.tight_layout()
    png_file = csv_file.replace(".csv", ".png")
    plt.savefig(png_file, dpi=150)
    print(f"PNG file: {png_file}")
    print(f"Data points: {len(timestamps)}")

def load_csv(csv_file):
    """CSV読み込み"""
    timestamps, angles, powers, powerLs, powerRs = [], [], [], [], []
    
    with open(csv_file, 'r') as f:
        for line in f:
            line = line.strip()
            if line.startswith("timestamp") or not line:
                continue
            
            parts = line.split(",")
            try:
                if parts[0].startswith("D"):
                    # D,Angle,power,powerL,powerR format (no timestamp)
                    # Estimate timestamp from line number
                    timestamps.append(len(timestamps) * 0.1)
                    angles.append(float(parts[1]))
                    powers.append(float(parts[2]))
                    if len(parts) >= 5:
                        powerLs.append(float(parts[3]))
                        powerRs.append(float(parts[4]))
                else:
                    # timestamp,D,Angle,power,powerL,powerR format
                    timestamps.append(float(parts[0]))
                    angles.append(float(parts[2]))
                    powers.append(float(parts[3]))
                    if len(parts) >= 6:
                        powerLs.append(float(parts[4]))
                        powerRs.append(float(parts[5]))
            except (ValueError, IndexError):
                continue
    
    return timestamps, angles, powers, powerLs, powerRs

if __name__ == "__main__":
    main()
