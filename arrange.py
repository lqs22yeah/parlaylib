import re
import json

def parse_test_data(data):
    results = []
    
    # Split into blocks based on `Running for n` and `skew`
    blocks = re.split(r"(Running for n = \d+|skew:Running for n = \d+)", data)
    blocks = [block.strip() for block in blocks if block.strip()]

    current_n = None
    current_result = None

    for i in range(0, len(blocks), 2):  # Process in pairs of [header, body]
        header = blocks[i]
        body = blocks[i + 1] if i + 1 < len(blocks) else ""

        # Debug: Print header and body for verification
        print(f"DEBUG: Processing block - header: {header}, body: {len(body)} characters")

        # Determine if it's non-skew or skew
        is_skew = header.startswith("skew:")
        n_match = re.search(r"(\d+)", header)
        if n_match:
            n_value = int(n_match.group(1))
            if current_result is None or current_n != n_value:
                if current_result:  # Save previous result if any
                    results.append(current_result)
                current_result = {"n": n_value, "non_skew": {}, "skew": {}}
                current_n = n_value

        # Ensure target_key is valid
        if current_result is None:
            raise ValueError(f"Error: current_result is not initialized. Problematic header: {header}")

        target_key = "skew" if is_skew else "non_skew"

        # Extract test times
        def extract_times(block):
            tests = {
                "TestIntegerSortInplaceUniquePtr": {},
                "TestIntegerSortCopyAndDestructiveMove": {}
            }
            # Extract TestIntegerSortInplaceUniquePtr data
            inplace_block = re.search(r"TestIntegerSortInplaceUniquePtr(.*?)(?=TestIntegerSortCopyAndDestructiveMove|$)", block, re.DOTALL)
            if inplace_block:
                block_text = inplace_block.group(1)
                tests["TestIntegerSortInplaceUniquePtr"]["Standard sort time"] = int(re.search(r"Standard sort time: (\d+) ms", block_text).group(1)) if re.search(r"Standard sort time: (\d+) ms", block_text) else None
                tests["TestIntegerSortInplaceUniquePtr"]["Parlay integer_sort time"] = int(re.search(r"Parlay integer_sort time: (\d+) ms", block_text).group(1)) if re.search(r"Parlay integer_sort time: (\d+) ms", block_text) else None

            # Extract TestIntegerSortCopyAndDestructiveMove data
            copy_move_block = re.search(r"TestIntegerSortCopyAndDestructiveMove(.*?)(?=Total test time|$)", block, re.DOTALL)
            if copy_move_block:
                block_text = copy_move_block.group(1)
                tests["TestIntegerSortCopyAndDestructiveMove"]["Standard sort time"] = int(re.search(r"Standard sort time: (\d+) ms", block_text).group(1)) if re.search(r"Standard sort time: (\d+) ms", block_text) else None
                tests["TestIntegerSortCopyAndDestructiveMove"]["Parlay integer_sort time"] = int(re.search(r"Parlay integer_sort time: (\d+) ms", block_text).group(1)) if re.search(r"Parlay integer_sort time: (\d+) ms", block_text) else None

            return tests

        current_result[target_key] = extract_times(body)

    # Add the last result
    if current_result:
        results.append(current_result)

    return results

def main():
    input_file = "integer_sorting_results.txt"  # Replace with your file name
    with open(input_file, "r") as f:
        raw_data = f.read()
    
    parsed_results = parse_test_data(raw_data)
    
    # Output the results in JSON format
    output_file = "parsed_sorting_times.json"
    with open(output_file, "w") as f:
        json.dump(parsed_results, f, indent=4)

    print(f"Parsed data has been saved to {output_file}")

if __name__ == "__main__":
    main()
