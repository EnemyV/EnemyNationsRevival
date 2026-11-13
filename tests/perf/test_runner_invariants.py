"""Runner qualification and environment isolation invariants."""
from run_fixture import observed_requested_duration, scrub_inherited_en

assert not observed_requested_duration([], 60)
assert not observed_requested_duration([{'monotonic': 1}], 60)
assert not observed_requested_duration([{'monotonic': 0}, {'monotonic': 60}], 720)
assert observed_requested_duration([{'monotonic': 3}, {'monotonic': 723}], 720)
env, removed = scrub_inherited_en({'EN_PATH_ASYNC': '1', 'en_hidden': 'secret', 'PATH': 'path', 'OTHER': 'ok'})
assert env == {'PATH': 'path', 'OTHER': 'ok'}
assert removed == ['EN_PATH_ASYNC', 'en_hidden']
print('requested-span and inherited EN isolation checks passed')
