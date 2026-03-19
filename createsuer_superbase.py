import requests

SUPABASE_URL = "https://nrtjpzkrldwydkbopsml.supabase.co"
SERVICE_ROLE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Im5ydGpwemtybGR3eWRrYm9wc21sIiwicm9sZSI6InNlcnZpY2Vfcm9sZSIsImlhdCI6MTc3Mzg1NTQ1NCwiZXhwIjoyMDg5NDMxNDU0fQ.W1vSmTFrsAiPWqqKmDjprMUavdzwxOYruHawA5MvnOU"

url = f"{SUPABASE_URL}/auth/v1/admin/users"

response = requests.post(
    url,
    headers={
        "apikey": SERVICE_ROLE_KEY,
        "Authorization": f"Bearer {SERVICE_ROLE_KEY}",
        "Content-Type": "application/json"
    },
    json={
        "email": "paul.mckinney@gmail.com",
        "password": "DOG$BITEME",
        "email_confirm": True  # 👈 skips confirmation
    }
)

data = response.json()
print(data)