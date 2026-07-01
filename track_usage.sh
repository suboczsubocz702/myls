#!/bin/bash

TARGET_DIR="${/home/arch/.}"

echo "Rozpoczęto automatyczne monitorowanie użycia plików w: $TARGET_DIR"

inotifywait -m -r -e access --format '%w%f' "$TARGET_DIR" | while read -r FILE
do
    if [[ "$FILE" == *"/.git"* || "$(basename "$FILE")" == .* || "$(basename "$FILE")" == "myls" ]]; then
        continue
    fi

    if [ -f "$FILE" ]; then
        CURRENT_COUNT=$(getfattr --absolute-names --only-values -n user.use_count "$FILE" 2>/dev/null)
        if [ -z "$CURRENT_COUNT" ]; then
            CURRENT_COUNT=0
        fi

        NEW_COUNT=$((CURRENT_COUNT + 1))

        setfattr -n user.use_count -v "$NEW_COUNT" "$FILE" 2>/dev/null
    fi
done

