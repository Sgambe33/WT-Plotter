import requests
import json
from math import floor
import pandas as pd


def calcoloBR(rank: int):
    return round(1 + 0.3*rank + 0.1*floor((rank+1)/3), 1)

api_url = "https://github.com/gszabi99/War-Thunder-Datamine/raw/refs/heads/master/char.vromfs.bin_u/config/wpcost.blkx"  # Replace with your actual API endpoint
result = requests.get(api_url).json()
csv_url = "https://github.com/gszabi99/War-Thunder-Datamine/raw/refs/heads/master/lang.vromfs.bin_u/lang/units.csv"
df = pd.read_csv(csv_url, delimiter=';', encoding='utf-8')


vehicles = []
#delete key from dict   

result.pop("economicRankMax")


for item in result:
    try:
        item_name_en = df.loc[df['<ID|readonly|noverify>'] == item+"_shop"]["<English>"].values
        item_name_ru = df.loc[df['<ID|readonly|noverify>'] == item+"_shop"]["<Russian>"].values

        obj = {
            "identifier": item,
            "en": item_name_en[0] if len(item_name_en) > 0 and pd.notna(item_name_en[0]) else "N/A",
            "ru": item_name_ru[0] if len(item_name_ru) > 0 and pd.notna(item_name_ru[0]) else "N/A",
            "country": result[item].get('country', "unknown").replace("country_", ""),
            "rank": result[item].get('rank', "unknown"),
            "ab_br": calcoloBR(result[item].get('economicRankArcade', 0)),
            "rb_br": calcoloBR(result[item].get('economicRankHistorical', 0)),
            "sb_br": calcoloBR(result[item].get('economicRankSimulation', 0))
        }
        vehicles.append(obj)
    except Exception as e:
        print(f"Error processing item {item}: {e}")

with open("vehicles.json", "w", encoding="UTF-8") as f:
    json.dump(vehicles, f, separators=(',', ':'), ensure_ascii=False)

csv_url = "https://github.com/gszabi99/War-Thunder-Datamine/raw/refs/heads/master/lang.vromfs.bin_u/lang/missions_locations.csv"

df = pd.read_csv(csv_url, delimiter=';', encoding='utf-8')

locations = []
for index, row in df.iterrows():
    obj = {
        "identifier": row["<ID|readonly|noverify>"].replace("location/", ""),
        "en": row["<English>"] if pd.notna(row["<English>"]) else "N/A",
        "ru": row["<Russian>"] if pd.notna(row["<Russian>"]) else "N/A",
    }

    locations.append(obj)
with open("locations.json", "w", encoding="UTF-8") as f:
    json.dump(locations, f, separators=(',', ':'), ensure_ascii=False, sort_keys=True)