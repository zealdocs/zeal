for file in zeal*;
do
    mv "$file" "${file#XY zeal}"
done

