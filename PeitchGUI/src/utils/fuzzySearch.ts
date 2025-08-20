export function fuzzySearch(query: string, text: string): boolean {
    if (!query) return true;
    query = query.toLowerCase().replace(/\s/g, '');
    text = text.toLowerCase();
    
    let i = 0;
    let j = 0;

    while (i < query.length && j < text.length) {
        if (query[i] === text[j]) {
            i++;
        }
        j++;
    }

    return i === query.length;
}