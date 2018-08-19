int printf(char *fmt, ...);
int scanf(char *fmt, ...);
void *malloc(int size);

int main() {
	int i,n;
	int *a;

	printf("Enter num : ");
	scanf("%d",&n);

	a = malloc(4*n);

	for(i=0;i<n;i++) {
		a[i] = i*i;
	}

	for(i=0;i<n;i++) {
		printf("%d\n",a[i]);
	}

	return 0;
}
