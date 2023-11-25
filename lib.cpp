// Test whether basic functions work
int mul(int x, int y) {
  return x * y;
}

// Test whether loops work
int pow(int x, int y) { 
  int ans = 1;
  for (int i = 0; i < y; ++i)
    ans = mul(ans, x);
  return ans;
}

// Test whether function calls work
int cube(int x) { return pow(x, 3); }