#!/usr/bin/env python3
"""
Tải bộ UPC WLTP Pack Cycling từ CORA.RDR (Dataverse của CSUC).

  DOI      : 10.34810/data2395
  Giấy phép: CC BY 4.0  -> dùng được cả cho mục đích thương mại, chỉ cần ghi nguồn
  Nội dung : 412 file parquet, ~1,4 GB, pack 3P12S = 36 cell
             72 kênh nhiệt độ cell (Top + Bottom mỗi cell), 2,5 Hz

Chạy:  ai/.venv/bin/python ai/download_upc.py
Tải lại được: file nào đã có và đủ dung lượng thì bỏ qua.
"""
import json, sys, time, urllib.request
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

DOI = "doi:10.34810/data2395"
BASE = "https://dataverse.csuc.cat"
OUT = Path(__file__).parent / "data" / "upc_wltp"
WORKERS = 6          # đủ nhanh mà không làm phiền máy chủ của người ta


def manifest():
    """Lấy danh sách file. Cache lại để chạy lại không phải hỏi máy chủ."""
    cache = OUT.parent / "upc_manifest.json"
    if cache.exists():
        return json.loads(cache.read_text())
    url = f"{BASE}/api/datasets/:persistentId/?persistentId={DOI}"
    with urllib.request.urlopen(url, timeout=120) as r:
        d = json.load(r)
    if d.get("status") != "OK":
        sys.exit(f"API trả về lỗi: {str(d)[:200]}")
    files = [
        {"id": f["dataFile"]["id"],
         "name": f["dataFile"]["filename"],
         "size": f["dataFile"]["filesize"]}
        for f in d["data"]["latestVersion"]["files"]
    ]
    cache.parent.mkdir(parents=True, exist_ok=True)
    cache.write_text(json.dumps(files, indent=1))
    return files


def fetch(f):
    dest = OUT / f["name"]
    # đã tải đủ thì thôi; tải dở (thiếu byte) thì tải lại
    if dest.exists() and dest.stat().st_size == f["size"]:
        return "bỏ qua"
    tmp = dest.with_suffix(dest.suffix + ".part")
    url = f"{BASE}/api/access/datafile/{f['id']}"
    for attempt in range(4):
        try:
            with urllib.request.urlopen(url, timeout=300) as r, open(tmp, "wb") as w:
                while chunk := r.read(1 << 20):
                    w.write(chunk)
            if tmp.stat().st_size != f["size"]:
                raise IOError(f"thiếu byte: {tmp.stat().st_size} != {f['size']}")
            tmp.rename(dest)          # chỉ đổi tên khi đã đủ -> không bao giờ để lại file hỏng
            return "xong"
        except Exception as e:
            if attempt == 3:
                return f"HỎNG ({e})"
            time.sleep(2 * (attempt + 1))


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    files = manifest()
    total = sum(f["size"] for f in files)
    print(f"{len(files)} file, tổng {total/1e9:.2f} GB -> {OUT}")

    done = 0
    with ThreadPoolExecutor(WORKERS) as ex:
        for f, status in zip(files, ex.map(fetch, files)):
            done += 1
            if status.startswith("HỎNG"):
                print(f"  [{done}/{len(files)}] {f['name']}: {status}")
            elif done % 25 == 0 or done == len(files):
                have = sum(p.stat().st_size for p in OUT.glob("*.parquet"))
                print(f"  [{done}/{len(files)}] đã có {have/1e9:.2f} GB")

    have = sorted(OUT.glob("*.parquet"))
    print(f"\nHoàn tất: {len(have)}/{len(files)} file, "
          f"{sum(p.stat().st_size for p in have)/1e9:.2f} GB")
    if len(have) != len(files):
        sys.exit("THIẾU FILE — chạy lại script để tải nốt")


if __name__ == "__main__":
    main()
