import csv

def preprocess_lobster(input_path, output_path):
    with open(input_path, "r") as infile, open(output_path, "w", newline="") as outfile:
        reader = csv.reader(infile)
        writer = csv.writer(outfile)

        for row in reader:
            if not row or row[0].startswith("#"):
                continue

            time, otype, oid, size, price, direction = row
            otype = int(otype)

            if otype not in [1, 2, 3]:
                continue  # skip executions, hidden, halts

            # Convert price from int to float by dividing by 1000
            price = float(int(price)) / 10000.0

            # Write row back out with adjusted price
            writer.writerow([time, otype, oid, size, price, direction])

    print(f"✅ Cleaned CSV written to {output_path}")


# Example usage
if __name__ == "__main__":
    preprocess_lobster("LOBSTER_APPL-1.csv", "AAPL_cleaned.csv")
