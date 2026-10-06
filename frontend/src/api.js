import axios from "axios";

const API_URL =
    import.meta.env.VITE_API_URL ||
    "http://localhost:8000";

console.log("API URL:", API_URL);

const api = axios.create({
    baseURL: API_URL,
    headers: {
        "Content-Type": "application/json",
    },
});

export default api;