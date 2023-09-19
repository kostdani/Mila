# Mila
Implementation of compiler for a subset of Pascal language, called Mila.

## Examples
```pascal
program factorialRec;

function fact(n: integer): integer;
begin
    if (n = 0) then
        fact := 1
    else
        fact := n * fact(n - 1);
end;

var
    n: integer;

begin
    readln(n);
    writeln(fact(n));
end.
```

```pascal
program factorialCycle;

var
    n: integer;
    f: integer;
begin
    f := 1;
    readln(n);
    while(n >= 2) do begin
        f := f * n;
	dec(n);
    end;
    writeln(f);
end.
```
more examples in `samples` directory
 
