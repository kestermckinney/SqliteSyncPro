import requests

SUPABASE_URL = "https://nrtjpzkrldwydkbopsml.supabase.co"
SERVICE_ROLE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Im5ydGpwemtybGR3eWRrYm9wc21sIiwicm9sZSI6InNlcnZpY2Vfcm9sZSIsImlhdCI6MTc3Mzg1NTQ1NCwiZXhwIjoyMDg5NDMxNDU0fQ.W1vSmTFrsAiPWqqKmDjprMUavdzwxOYruHawA5MvnOU"

url = f"{SUPABASE_URL}/auth/v1/admin/users"

response = requests.get(
    url,
    headers={
        "apikey": SERVICE_ROLE_KEY,
        "Authorization": f"Bearer {SERVICE_ROLE_KEY}"
    }
)

users = response.json()

user_uuid = None
user_email = "paul.mckinney@gmail.com"

for user in users["users"]:
    print(user["id"], user["email"])
    if user["email"] == user_email:
        user_uuid = user["id"]

url = f"{SUPABASE_URL}/auth/v1/admin/users/{user_uuid}"

response = requests.delete(
    url,
    headers={
        "apikey": SERVICE_ROLE_KEY,
        "Authorization": f"Bearer {SERVICE_ROLE_KEY}",
        "Content-Type": "application/json"
    }
)

# Check result
if response.status_code == 200:
    print("✅ User deleted successfully")
else:
    print("❌ Failed to delete user")
    print(response.status_code, response.text)